/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include "sfiglueproxy.h"
#include "sfiglue.h"
#include "sfiustore.h"
#include "sfivcall.h"
#include <gobject/gvaluecollector.h>
#include <sfi/gbsearcharray.h>


/* --- macros --- */
#define	sfi_proxy_warn_inval(where,proxy)	g_message ("%s: invalid proxy id (%lu)", (where), (proxy))


/* --- structures --- */
typedef struct {
  GQuark     qsignal;
  GHookList *hlist;
} GlueSignal;
typedef struct {
  SfiProxy       proxy;
  GData         *qdata;
  GBSearchArray *signals;
} Proxy;
typedef struct {
  SfiGlueEvent	 type;
  SfiProxy	 proxy;
  gchar		*signal;
  SfiSeq	*args;
} GlueEvent;


/* --- prototypes --- */
static Proxy*	fetch_proxy	(SfiGlueContext	*context,
				 SfiProxy	 proxy);
static gint	signals_compare	(gconstpointer	 bsearch_node1, /* key */
				 gconstpointer   bsearch_node2);


/* --- variables --- */
static const GBSearchConfig signals_config = {
  sizeof (GlueSignal),
  signals_compare,
  G_BSEARCH_ARRAY_FORCE_SHRINK,
};


/* --- functions --- */
static gint
signals_compare (gconstpointer bsearch_node1, /* key */
		 gconstpointer bsearch_node2)
{
  const GlueSignal *s1 = bsearch_node1;
  const GlueSignal *s2 = bsearch_node1;
  return s1->qsignal < s2->qsignal ? -1 : s1->qsignal != s2->qsignal;
}

static inline Proxy*
peek_proxy (SfiGlueContext *context,
	    SfiProxy        proxy)
{
  return sfi_ustore_lookup (context->proxies, proxy);
}

static inline GlueSignal*
peek_signal (SfiGlueContext *context,
	     Proxy          *p,
	     GQuark          qsignal)
{
  if (qsignal)
    {
      GlueSignal key;
      key.qsignal = qsignal;
      return g_bsearch_array_lookup (p->signals, &signals_config, &key);
    }
  return NULL;
}

static void
remove_proxy (SfiGlueContext *context,
	      Proxy          *p)
{
  sfi_ustore_remove (context->proxies, p->proxy);
  g_free (p);
}

static void
free_hook_list (GHookList *hlist)
{
  g_hook_list_clear (hlist);
  g_free (hlist);
}

static void
delete_signal (SfiGlueContext *context,
	       Proxy	      *p,
	       GQuark          qsignal)
{
  GlueSignal *sig = peek_signal (context, p, qsignal);
  guint indx = g_bsearch_array_get_index (p->signals, &signals_config, sig);
  const gchar *signal = g_quark_to_string (sig->qsignal);
  sfi_glue_gc_add (sig->hlist, free_hook_list);
  p->signals = g_bsearch_array_remove (p->signals, &signals_config, indx);
  context->table.proxy_notify (context, p->proxy, signal, FALSE);
}

static GlueSignal*
fetch_signal (SfiGlueContext *context,
	      Proxy          *p,
	      const gchar    *signal)
{
  GQuark quark = g_quark_try_string (signal);
  GlueSignal key, *sig = NULL;

  if (quark)
    {
      key.qsignal = quark;
      sig = g_bsearch_array_lookup (p->signals, &signals_config, &key);
      if (sig)
	return sig;
    }
  if (!context->table.proxy_notify (context, p->proxy, signal, TRUE))
    return NULL;
  key.qsignal = g_quark_from_string (signal);
  key.hlist = g_new0 (GHookList, 1);
  g_hook_list_init (key.hlist, sizeof (GHook));
  p->signals = g_bsearch_array_insert (p->signals, &signals_config, &key);
  return g_bsearch_array_lookup (p->signals, &signals_config, &key);
}

static Proxy*
fetch_proxy (SfiGlueContext *context,
	     SfiProxy        proxy)
{
  Proxy *p = sfi_ustore_lookup (context->proxies, proxy);

  if (!p)
    {
      if (!context->table.proxy_watch_release (context, proxy))
	return NULL;
      p = g_new0 (Proxy, 1);
      p->proxy = proxy;
      g_datalist_init (&p->qdata);
      p->signals = g_bsearch_array_create (&signals_config);
      sfi_ustore_insert (context->proxies, proxy, p);
    }
  return p;
}

void
_sfi_glue_proxy_release (SfiGlueContext *context,
			 SfiProxy        proxy)
{
  Proxy *p = peek_proxy (context, proxy);

  g_return_if_fail (proxy != 0);

  if (p)
    {
      Proxy tmp = *p;
      guint i;
      remove_proxy (context, p);	/* early unlink */
      p = &tmp;
      i = g_bsearch_array_get_n_nodes (p->signals);
      while (i)
	{
	  GlueSignal *sig = g_bsearch_array_get_nth (p->signals, &signals_config, --i);
	  delete_signal (context, p, sig->qsignal);
	}
      g_bsearch_array_free (p->signals, &signals_config);
      g_datalist_clear (&p->qdata);
    }
  else
    sfi_proxy_warn_inval (G_STRLOC, proxy);
}

void
_sfi_glue_proxy_signal (SfiGlueContext *context,
			SfiProxy        proxy,
			const gchar    *signal,
			SfiSeq         *args)
{
  Proxy *p;

  g_return_if_fail (proxy > 0 && signal);

  p = peek_proxy (context, proxy);
  if (p)
    {
      GlueSignal *sig = peek_signal (context, p, g_quark_try_string (signal));
      if (sig)
	{
	  GHookList *hlist = sig->hlist;
	  GHook *hook;
	  sig = NULL;	/* may mutate during callbacks */
	  hook = g_hook_first_valid (hlist, TRUE);
	  while (hook)
	    {
	      gboolean was_in_call = G_HOOK_IN_CALL (hook);

	      hook->flags |= G_HOOK_FLAG_IN_CALL;
	      g_closure_invoke (hook->data, NULL, args->n_elements, args->elements, (gpointer) signal);
	      if (!was_in_call)
		hook->flags &= ~G_HOOK_FLAG_IN_CALL;
	      hook = g_hook_next_valid (hlist, hook, TRUE);
	    }
	}
      else
	g_warning ("spurious unknown signal \"%s\" on proxy (%lu)", signal, proxy);
    }
  else
    g_warning ("spurious signal \"%s\" on non existing proxy (%lu)", signal, proxy);
}

static void
default_glue_marshal (GClosure       *closure,
		      GValue /*out*/ *return_value,
		      guint           n_param_values,
		      const GValue   *param_values,
		      gpointer        invocation_hint,
		      gpointer        marshal_data)
{
  g_return_if_fail (return_value == NULL);
  g_return_if_fail (n_param_values > 0);
  g_return_if_fail (SFI_VALUE_HOLDS_PROXY (param_values));

  sfi_vcall_void (((GCClosure*) closure)->callback,
		  (gpointer) sfi_value_get_proxy (param_values),
		  n_param_values - 1,
		  param_values + 1,
		  closure->data);
}

gulong
sfi_glue_signal_connect (SfiProxy       proxy,
			 const gchar   *signal,
			 GClosure      *closure,
			 gpointer       search_data)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  Proxy *p;

  g_return_val_if_fail (proxy > 0, 0);
  g_return_val_if_fail (signal != NULL, 0);
  g_return_val_if_fail (closure != NULL, 0);

  g_closure_ref (closure);
  g_closure_sink (closure);

  p = fetch_proxy (context, proxy);
  if (!p)
    {
      sfi_proxy_warn_inval (G_STRLOC, proxy);
      sfi_glue_gc_add (closure, g_closure_unref);
    }
  else
    {
      GlueSignal *sig = fetch_signal (context, p, signal);
      if (sig)
	{
	  GHook *hook = g_hook_alloc (sig->hlist);
	  hook->data = closure;
	  hook->destroy = (GDestroyNotify) g_closure_unref;
	  hook->func = search_data;
	  if (G_CLOSURE_NEEDS_MARSHAL (closure))
	    g_closure_set_marshal (closure, default_glue_marshal);
	  sig->hlist->seq_id = context->seq_hook_id;
	  g_hook_append (sig->hlist, hook);
	  context->seq_hook_id = sig->hlist->seq_id;
	  return hook->hook_id;
	}
      else
	sfi_glue_gc_add (closure, g_closure_unref);
    }
  return 0;
}

void
sfi_glue_signal_disconnect (SfiProxy     proxy,
			    gulong       connection_id)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  Proxy *p;

  g_return_if_fail (proxy > 0);
  g_return_if_fail (connection_id > 0);

  p = fetch_proxy (context, proxy);
  if (!p)
    {
      sfi_proxy_warn_inval (G_STRLOC, proxy);
      return;
    }
  else
    {
      guint i;
      for (i = 0; i < g_bsearch_array_get_n_nodes (p->signals); i++)
	{
	  GlueSignal *sig = g_bsearch_array_get_nth (p->signals, &signals_config, i);
	  GHookList *hlist = sig->hlist;
	  GQuark qsignal = sig->qsignal;
	  sig = NULL;	/* mutates during callback */
	  if (g_hook_destroy (hlist, connection_id))	/* callback */
	    {
	      GHook *hook = g_hook_first_valid (hlist, TRUE);
	      /* figure whether this was the last valid connection */
	      if (hook)
		g_hook_unref (hlist, hook);
	      else
		delete_signal (context, p, qsignal);
	      return;
	    }
	}
    }
  g_message ("proxy (%lu) has no signal connection (%lu) to disconnect",
	     proxy, connection_id);
}

GSList*
_sfi_glue_signal_find_closures (SfiGlueContext *context,
				SfiProxy        proxy,
				const gchar    *signal,
				gpointer	closure_data,
				gpointer        search_data)
{
  GSList *ids = NULL;
  Proxy *p;

  g_return_val_if_fail (proxy > 0, NULL);
  g_return_val_if_fail (search_data != NULL, NULL);

  p = fetch_proxy (context, proxy);
  if (p && signal)
    {
      GlueSignal *sig = peek_signal (context, p, g_quark_try_string (signal));
      if (sig)
	{
	  GHook *hook = sig->hlist->hooks;
	  while (hook)
	    {
	      if (G_HOOK_IS_VALID (hook) && /* test only non-destroyed hooks */
		  hook->func == search_data &&
		  ((GClosure*) hook->data)->data == closure_data)
		ids = g_slist_prepend (ids, (gpointer) hook->hook_id);
	      hook = hook->next;
	    }
	}
    }
  else if (p)
    {
      guint i;
      for (i = 0; i < g_bsearch_array_get_n_nodes (p->signals); i++)
	{
	  GlueSignal *sig = g_bsearch_array_get_nth (p->signals, &signals_config, i);
	  GHook *hook = sig->hlist->hooks;
	  while (hook)
	    {
	      if (G_HOOK_IS_VALID (hook) && /* test only non-destroyed hooks */
		  hook->func == search_data &&
		  ((GClosure*) hook->data)->data == closure_data)
		ids = g_slist_prepend (ids, (gpointer) hook->hook_id);
	      hook = hook->next;
	    }
	}
    }

  return ids;
}

void
sfi_glue_proxy_connect (SfiProxy     proxy,
			const gchar *signal,
			...)
{
  va_list var_args;

  g_return_if_fail (proxy > 0);

  va_start (var_args, signal);
  while (signal)
    {
      gpointer callback = va_arg (var_args, gpointer);
      gpointer data = va_arg (var_args, gpointer);

      if (strncmp (signal, "signal::", 8) == 0)
	sfi_glue_signal_connect (proxy, signal + 8,
				 g_cclosure_new (callback, data, NULL), callback);
      else if (strncmp (signal, "object_signal::", 15) == 0 ||
	       strncmp (signal, "object-signal::", 15) == 0)
	sfi_glue_signal_connect (proxy, signal + 15,
				 g_cclosure_new_object (callback, data), callback);
      else if (strncmp (signal, "swapped_signal::", 16) == 0 ||
	       strncmp (signal, "swapped-signal::", 16) == 0)
	sfi_glue_signal_connect (proxy, signal + 16,
				 g_cclosure_new_swap (callback, data, NULL), callback);
      else if (strncmp (signal, "swapped_object_signal::", 23) == 0 ||
	       strncmp (signal, "swapped-object-signal::", 23) == 0)
	sfi_glue_signal_connect (proxy, signal + 23,
				 g_cclosure_new_object_swap (callback, data), callback);
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRLOC, signal);
	  break;
	}
      signal = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

void
sfi_glue_proxy_disconnect (SfiProxy     proxy,
			   const gchar *signal,
			   ...)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  va_list var_args;

  g_return_if_fail (proxy > 0);

  va_start (var_args, signal);
  while (signal)
    {
      gpointer callback = va_arg (var_args, gpointer);
      gpointer data = va_arg (var_args, gpointer);
      GSList *node, *slist = NULL;

      if (strncmp (signal, "any_signal::", 12) == 0)
	{
	  signal += 12;
	  slist = _sfi_glue_signal_find_closures (context, proxy, signal, data, callback);
	  for (node = slist; node; node = node->next)
	    sfi_glue_signal_disconnect (proxy, (gulong) node->data);
	  g_slist_free (slist);
	}
      else if (strcmp (signal, "any_signal") == 0)
	{
	  slist = _sfi_glue_signal_find_closures (context, proxy, NULL, data, callback);
	  for (node = slist; node; node = node->next)
	    sfi_glue_signal_disconnect (proxy, (gulong) node->data);
	  g_slist_free (slist);
	}
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRLOC, signal);
	  break;
	}

      if (!slist)
	g_warning ("%s: signal handler %p(%p) is not connected", G_STRLOC, callback, data);
      signal = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

gpointer
sfi_glue_proxy_get_qdata (SfiProxy proxy,
			  GQuark   quark)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  Proxy *p = peek_proxy (context, proxy);

  g_return_val_if_fail (proxy != 0, NULL);

  return p && quark ? g_datalist_id_get_data (&p->qdata, quark) : NULL;
}

gpointer
sfi_glue_proxy_steal_qdata (SfiProxy proxy,
			    GQuark   quark)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  Proxy *p = peek_proxy (context, proxy);

  g_return_val_if_fail (proxy != 0, NULL);

  return p && quark ? g_datalist_id_remove_no_notify (&p->qdata, quark) : NULL;
}

void
sfi_glue_proxy_set_qdata_full (SfiProxy       proxy,
			       GQuark         quark,
			       gpointer       data,
			       GDestroyNotify destroy)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  Proxy *p;

  g_return_if_fail (proxy != 0);
  g_return_if_fail (quark != 0);

  p = fetch_proxy (context, proxy);
  if (!p)
    {
      sfi_proxy_warn_inval (G_STRLOC, proxy);
      if (destroy)
	sfi_glue_gc_add (data, destroy);
    }
  else
    g_datalist_id_set_data_full (&p->qdata, quark, data, data ? destroy : NULL);
}

const gchar*
sfi_glue_proxy_iface (SfiProxy proxy)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar *iface;

  g_return_val_if_fail (proxy != 0, NULL);

  iface = context->table.proxy_iface (context, proxy);

  if (iface)
    sfi_glue_gc_add (iface, g_free);
  return iface;
}

gboolean
sfi_glue_proxy_is_a (SfiProxy     proxy,
		     const gchar *type)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);

  g_return_val_if_fail (proxy != 0, FALSE);
  g_return_val_if_fail (type != NULL, FALSE);

  return context->table.proxy_is_a (context, proxy, type);
}

GParamSpec*
sfi_glue_proxy_get_pspec (SfiProxy     proxy,
			  const gchar *name)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GParamSpec *pspec;

  g_return_val_if_fail (proxy != 0, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  pspec = context->table.proxy_get_pspec (context, proxy, name);
  if (pspec)
    sfi_glue_gc_add (pspec, g_param_spec_unref);
  return pspec;
}

const gchar**
sfi_glue_proxy_list_properties (SfiProxy     proxy,
				const gchar *first_ancestor,
				const gchar *last_ancestor,
				guint       *n_props)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  gchar **props;

  g_return_val_if_fail (proxy != 0, NULL);

  if (first_ancestor && !first_ancestor[0])
    first_ancestor = NULL;
  if (last_ancestor && !last_ancestor[0])
    last_ancestor = NULL;

  props = context->table.proxy_list_properties (context, proxy, first_ancestor, last_ancestor);
  if (!props)
    props = g_new0 (gchar*, 1);
  sfi_glue_gc_add (props, g_strfreev);
  if (n_props)
    {
      guint i = 0;
      while (props[i])
	i++;
      *n_props = i;
    }
  return (const gchar**) props;
}

void
sfi_glue_proxy_set_property (SfiProxy      proxy,
			     const gchar  *prop,
			     const GValue *value)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);

  g_return_if_fail (proxy != 0);
  g_return_if_fail (prop != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  context->table.proxy_set_property (context, proxy, prop, value);
}

const GValue*
sfi_glue_proxy_get_property (SfiProxy     proxy,
			     const gchar *prop)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GValue *value;

  g_return_val_if_fail (proxy != 0, NULL);
  g_return_val_if_fail (prop != NULL, NULL);

  value = context->table.proxy_get_property (context, proxy, prop);
  if (value)
    sfi_glue_gc_add (value, sfi_value_free);
  return value;
}

void
sfi_glue_proxy_set (SfiProxy     proxy,
		    const gchar *prop,
		    ...)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  va_list var_args;

  g_return_if_fail (proxy != 0);

  va_start (var_args, prop);
  while (prop)
    {
      SfiSCategory scat = context->table.proxy_get_pspec_scategory (context, proxy, prop);
      GType vtype = sfi_category_type (scat);
      gchar *error = NULL;
      if (vtype)
	{
	  GValue value = { 0, };
	  g_value_init (&value, vtype);
	  G_VALUE_COLLECT (&value, var_args, G_VALUE_NOCOPY_CONTENTS, &error);
	  if (!error)
	    context->table.proxy_set_property (context, proxy, prop, &value);
	  g_value_unset (&value);
	}
      else
	error = g_strdup_printf ("unknown property \"%s\"", prop);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  break;
	}
      prop = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

void
sfi_glue_proxy_get (SfiProxy     proxy,
		    const gchar *prop,
		    ...)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  va_list var_args;

  g_return_if_fail (proxy != 0);

  va_start (var_args, prop);
  while (prop)
    {
      GValue *value = context->table.proxy_get_property (context, proxy, prop);
      gchar *error = NULL;
      if (value)
	{
	  sfi_glue_gc_add (value, sfi_value_free);
	  G_VALUE_LCOPY (value, var_args, G_VALUE_NOCOPY_CONTENTS, &error);
	}
      else
        error = g_strdup_printf ("unknown property \"%s\"", prop);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  break;
	}
      prop = va_arg (var_args, gchar*);
    }
  va_end (var_args);
}

void
sfi_glue_enqueue_event (SfiGlueEvent event_type,
			SfiSeq      *aseq)
{
  SfiGlueContext *context = sfi_glue_fetch_context (G_STRLOC);
  GlueEvent e = { 0, };

  g_return_if_fail (aseq != NULL);

  switch (event_type)
    {
      GValue *field1, *field2, *element1;
    case SFI_GLUE_EVENT_RELEASE:
      g_return_if_fail (aseq->n_elements == 1);
      field1 = sfi_seq_get (aseq, 0);
      g_return_if_fail (SFI_VALUE_HOLDS_PROXY (field1));
      e.type = SFI_GLUE_EVENT_RELEASE;
      e.proxy = sfi_value_get_proxy (field1);
      e.signal = NULL;
      e.args = NULL;
      break;
    case SFI_GLUE_EVENT_SIGNAL:
      g_return_if_fail (aseq->n_elements == 2);
      field1 = sfi_seq_get (aseq, 0);
      field2 = sfi_seq_get (aseq, 1);
      g_return_if_fail (SFI_VALUE_HOLDS_STRING (field1));
      g_return_if_fail (SFI_VALUE_HOLDS_SEQ (field2));
      e.args = sfi_value_get_seq (field2);
      g_return_if_fail (e.args != NULL);
      element1 = sfi_seq_get (e.args, 0);
      g_return_if_fail (SFI_VALUE_HOLDS_PROXY (element1));
      e.type = SFI_GLUE_EVENT_SIGNAL;
      e.proxy = sfi_value_get_proxy (element1);
      e.signal = sfi_value_dup_string (field1);
      e.args = sfi_seq_copy_deep (e.args);
      break;
    default:
      g_warning ("%s: invalid event type (%u)", G_STRLOC, event_type);
      return;
    }
  context->events = sfi_ring_append (context->events, g_memdup (&e, sizeof (e)));
}

void
_sfi_glue_proxy_dispatch (SfiGlueContext *context)
{
  static gboolean glue_proxy_dispatching = FALSE;

  g_return_if_fail (glue_proxy_dispatching == FALSE);

  glue_proxy_dispatching = TRUE;

  while (context->events)
    {
      GlueEvent *event = sfi_ring_pop_head (&context->events);

      switch (event->type)
	{
	case SFI_GLUE_EVENT_RELEASE:
	  _sfi_glue_proxy_release (context, event->proxy);
	  break;
	case SFI_GLUE_EVENT_SIGNAL:
	  _sfi_glue_proxy_signal (context, event->proxy, event->signal, event->args);
	  g_free (event->signal);
	  sfi_seq_unref (event->args);
	  break;
	}
      g_free (event);
    }

  glue_proxy_dispatching = FALSE;
}

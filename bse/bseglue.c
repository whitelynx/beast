/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bseglue.h"
#include "bse.h"
#include <string.h>


/* --- structures --- */
typedef struct {
  SfiGlueContext context;
  gchar         *user;
  SfiUStore     *proxies;
} BContext;
typedef struct {
  gulong  release_id;
  guint   remote_watch : 1;
  GSList *closures;
} BProxy;
typedef struct {
  GClosure  closure;
  GQuark    qsignal;
  gulong    handler_id;
} BClosure;


/* --- prototypes --- */
static SfiGlueIFace*    bglue_describe_iface            (SfiGlueContext *context,
							 const gchar    *iface);
static SfiGlueProc*     bglue_describe_proc             (SfiGlueContext *context,
							 const gchar    *proc_name);
static gchar**          bglue_list_proc_names           (SfiGlueContext *context);
static gchar**          bglue_list_method_names         (SfiGlueContext *context,
							 const gchar    *iface_name);
static gchar*           bglue_base_iface                (SfiGlueContext *context);
static gchar**          bglue_iface_children            (SfiGlueContext *context,
							 const gchar    *iface_name);
static GValue*          bglue_exec_proc                 (SfiGlueContext *context,
							 const gchar    *proc_name,
							 SfiSeq         *params);
static gchar*           bglue_proxy_iface               (SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean         bglue_proxy_is_a                (SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *iface);
static gchar**		bglue_proxy_list_properties	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *first_ancestor,
							 const gchar    *last_ancestor);
static GParamSpec*	bglue_proxy_get_pspec		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop_name);
static SfiSCategory	bglue_proxy_get_pspec_scategory	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop_name);
static void		bglue_proxy_set_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop,
							 const GValue   *value);
static GValue*		bglue_proxy_get_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop);
static gboolean		bglue_proxy_watch_release	(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean		bglue_proxy_notify		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *signal,
							 gboolean        enable_notify);
static GValue*          bglue_client_msg                (SfiGlueContext *context,
							 const gchar    *msg,
							 GValue         *value);


/* --- variables --- */
static GQuark quark_original_enum = 0;


/* --- functions --- */
SfiGlueContext*
bse_glue_context (const gchar *user)
{
  static const SfiGlueContextTable bse_glue_table = {
    bglue_describe_iface,
    bglue_describe_proc,
    bglue_list_proc_names,
    bglue_list_method_names,
    bglue_base_iface,
    bglue_iface_children,
    bglue_exec_proc,
    bglue_proxy_iface,
    bglue_proxy_is_a,
    bglue_proxy_list_properties,
    bglue_proxy_get_pspec,
    bglue_proxy_get_pspec_scategory,
    bglue_proxy_set_property,
    bglue_proxy_get_property,
    bglue_proxy_watch_release,
    bglue_proxy_notify,
    bglue_client_msg,
  };
  BContext *bcontext;

  g_return_val_if_fail (user != NULL, NULL);

  bcontext = g_new0 (BContext, 1);
  sfi_glue_context_common_init (&bcontext->context, &bse_glue_table);
  bcontext->user = g_strdup (user);
  bcontext->proxies = sfi_ustore_new ();

  if (!quark_original_enum)
    quark_original_enum = g_quark_from_static_string ("bse-glue-original-enum");
  
  return &bcontext->context;
}

static void
bcontext_queue_release (BContext *bcontext,
			SfiProxy  proxy)
{
  SfiSeq *seq, *event;

  seq = sfi_seq_new ();
  sfi_seq_append_proxy (seq, proxy);
  event = sfi_seq_new ();
  sfi_seq_append_int (event, SFI_GLUE_EVENT_RELEASE);
  sfi_seq_append_seq (event, seq);
  sfi_seq_unref (seq);
  /* FIXME: send event along */
  sfi_seq_unref (event);
}

static void
bcontext_queue_signal (BContext    *bcontext,
		       const gchar *signal,
		       SfiSeq      *args)
{
  SfiSeq *seq, *event;

  g_return_if_fail (args != NULL && args->n_elements > 0 && SFI_VALUE_HOLDS_PROXY (args->elements));

  seq = sfi_seq_new ();
  sfi_seq_append_string (seq, signal);
  sfi_seq_append_seq (seq, args);
  event = sfi_seq_new ();
  sfi_seq_append_int (event, SFI_GLUE_EVENT_SIGNAL);
  sfi_seq_append_seq (event, seq);
  sfi_seq_unref (seq);
  /* FIXME: send event along */
  sfi_seq_unref (event);
}

static GParamSpec*
pspec_to_serializable (GParamSpec *pspec)
{
  if (BSE_IS_PARAM_SPEC_ENUM (pspec))
    {
      GType etype = G_PARAM_SPEC_VALUE_TYPE (pspec);
      pspec = sfi_pspec_choice_from_enum (pspec);
      g_param_spec_set_qdata (pspec, quark_original_enum, (gpointer) etype);
    }
  else
    pspec = sfi_pspec_to_serializable (pspec);
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  return pspec;
}

static GValue*
value_from_serializable (const GValue *svalue,
			 GParamSpec   *pspec)
{
  GType dtype = 0, vtype = G_VALUE_TYPE (svalue);
  GValue *value = NULL;
  /* this corresponds with the conversions in sfi_pspec_to_serializable() */
  if (sfi_categorize_pspec (pspec))
    return NULL;
  if (SFI_VALUE_HOLDS_CHOICE (svalue) && G_IS_PARAM_SPEC_ENUM (pspec))
    {
      value = sfi_value_empty ();
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      sfi_value_choice2enum (svalue, value, pspec);
      return value;
    }
  else if (G_IS_PARAM_SPEC_BOXED (pspec) && (SFI_VALUE_HOLDS_SEQ (svalue) ||
					     SFI_VALUE_HOLDS_REC (svalue)))
    dtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  else if (SFI_VALUE_HOLDS_PROXY (svalue) && BSE_IS_PARAM_SPEC_OBJECT (pspec))
    {
      SfiProxy proxy = sfi_value_get_proxy (svalue);
      value = sfi_value_empty ();
      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      bse_value_set_object (value, bse_object_from_id (proxy));
      return value;
    }
  if (dtype)
    {
      value = sfi_value_empty ();
      g_value_init (value, dtype);
    }
  if (!dtype || !g_value_transform (svalue, value))
    g_warning ("unable to convert to value type `%s' from serializable (`%s')",
	       dtype ? g_type_name (dtype) : "<unknown>", g_type_name (vtype));
  return value;
}

static GValue*
value_to_serializable (const GValue *svalue)
{
  GValue *value = NULL;
  GType dtype = 0, vtype = G_VALUE_TYPE (svalue);
  /* this corresponds with the conversions in sfi_pspec_to_serializable() */
  if (sfi_categorize_type (vtype))
    return sfi_value_clone_shallow (svalue);

  switch (G_TYPE_FUNDAMENTAL (vtype))
    {
      BseObject *object;
    case G_TYPE_BOXED:
      if (sfi_boxed_get_record_info (vtype))
	dtype = SFI_TYPE_REC;
      else if (sfi_boxed_get_sequence_info (vtype))
	dtype = SFI_TYPE_SEQ;
      break;
    case G_TYPE_ENUM:
      dtype = SFI_TYPE_CHOICE;
      break;
    case G_TYPE_OBJECT:
      object = bse_value_get_object (svalue);
      return sfi_value_proxy (BSE_IS_OBJECT (object) ? BSE_OBJECT_ID (object) : 0);
    }
  if (dtype)
    {
      value = sfi_value_empty ();
      g_value_init (value, dtype);
    }
  if (!dtype || !g_value_transform (svalue, value))
    g_warning ("unable to convert value type `%s' to serializable (`%s')",
	       g_type_name (vtype), dtype ? g_type_name (dtype) : "<unknown>");
  return value;
}

GType
bse_glue_make_rorecord (const gchar      *rec_name,
			GBoxedCopyFunc    copy,
			GBoxedFreeFunc    free,
			BseGlueBoxedToRec to_record)
{
  GType type;
  
  type = g_boxed_type_register_static (rec_name, copy, free);
  g_type_set_qdata (type, g_quark_from_string ("BseGlueBoxedToRec"), to_record);
  
  return type;
}

GType
bse_glue_make_rosequence (const gchar      *seq_name,
			  GBoxedCopyFunc    copy,
			  GBoxedFreeFunc    free,
			  BseGlueBoxedToSeq to_sequence)
{
  GType type;
  
  type = g_boxed_type_register_static (seq_name, copy, free);
  g_type_set_qdata (type, g_quark_from_string ("BseGlueBoxedToSeq"), to_sequence);
  
  return type;
}

/**
 * BseGlueBoxedToRec
 * @boxed:   the boxed value to be converted into a record
 * @RETURNS: a GC owned SfiRec*
 *
 * Construct a new #SfiRec from a boxed value.
 */
/**
 * BseGlueBoxedToSeq
 * @boxed:   the boxed value to be converted into a sequence
 * @RETURNS: a GC owned SfiSeq*
 *
 * Construct a new #SfiSeq from a boxed value.
 */
/**
 * bse_glue_boxed_to_value
 * @boxed_type: type of the boxed value
 * @boxed:      the boxed value
 *
 * Covert a boxed value into a #SfiGlueValue (usually holding
 * either a sequence or a record). The returned value is owned
 * by the GC.
 */
GValue*
bse_glue_boxed_to_value (GType    boxed_type,
			 gpointer boxed)
{
  BseGlueBoxedToRec b2rec;
  BseGlueBoxedToSeq b2seq;
  GValue *value;
  
  g_return_val_if_fail (G_TYPE_IS_BOXED (boxed_type) && G_TYPE_IS_DERIVED (boxed_type), NULL);
  g_return_val_if_fail (boxed != NULL, NULL);
  
  b2rec = g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToRec"));
  b2seq = g_type_get_qdata (boxed_type, g_quark_from_string ("BseGlueBoxedToSeq"));
  if (b2rec)
    {
      SfiRec *rec = b2rec (boxed);
      value = sfi_value_rec (rec);
      sfi_rec_unref (rec);
    }
  else if (b2seq)
    {
      SfiSeq *seq = b2seq (boxed);
      value = sfi_value_seq (seq);
      sfi_seq_unref (seq);
    }
  else /* urm, bad */
    {
      g_warning ("unable to convert boxed type `%s' to record or sequence", g_type_name (boxed_type));
      value = NULL;
    }
  return value;
}

GType
bse_glue_pspec_get_original_enum (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);
  return (GType) g_param_spec_get_qdata (pspec, quark_original_enum);
}

static SfiGlueIFace*
bglue_describe_iface (SfiGlueContext *context,
                      const gchar    *iface)
{
  GType xtype, type = g_type_from_name (iface);
  SfiGlueIFace *f;
  GObjectClass *oclass;
  GParamSpec **pspecs;
  GSList *plist = NULL;
  guint i, n;
  
  if (!G_TYPE_IS_OBJECT (type) || !g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  f = _sfi_glue_iface_new (g_type_name (type));
  f->n_ifaces = g_type_depth (type) - g_type_depth (BSE_TYPE_ITEM) + 1;
  f->ifaces = g_new (gchar*, f->n_ifaces + 1);
  xtype = type;
  for (i = 0; i < f->n_ifaces; i++)
    {
      f->ifaces[i] = g_strdup (g_type_name (xtype));
      xtype = g_type_parent (xtype);
    }
  f->ifaces[i] = NULL;
  
  oclass = g_type_class_ref (type);
  xtype = BSE_TYPE_ITEM;
  pspecs = g_object_class_list_properties (oclass, &n);
  f->n_props = 0;
  for (i = 0; i < n; i++)
    {
      GParamSpec *pspec = pspecs[i];
      
      if (g_type_is_a (pspec->owner_type, xtype))
        {
          plist = g_slist_prepend (plist, g_strdup (pspec->name));
          f->n_props++;
        }
    }
  g_free (pspecs);
  g_type_class_unref (oclass);
  
  i = f->n_props;
  f->props = g_new (gchar*, i + 1);
  f->props[i] = NULL;
  while (i--)
    {
      GSList *tmp = plist->next;
      
      f->props[i] = plist->data;
      g_slist_free_1 (plist);
      plist = tmp;
    }
  
  f->n_signals = f->n_props;
  f->signals = g_new (gchar*, f->n_signals + 1);
  for (i = 0; i < f->n_props; i++)
    {
      gchar *signame = g_strdup_printf ("notify::%s", f->props[i]);
      
      f->signals[i] = g_strdup (signame);
      g_free (signame);
    }
  f->signals[i] = NULL;
  
  return f;
}

guint
bse_glue_enum_index (GType enum_type,
		     gint  enum_value)
{
  GEnumClass *eclass;
  GEnumValue *ev;
  guint index;
  
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), G_MAXINT);
  g_return_val_if_fail (G_TYPE_IS_DERIVED (enum_type), G_MAXINT);
  
  eclass = g_type_class_ref (enum_type);
  ev = g_enum_get_value (eclass, enum_value);
  if (!ev)
    g_message ("%s: enum \"%s\" has no value %u", G_STRLOC, g_type_name (enum_type), enum_value);
  index = ev ? ev - eclass->values : G_MAXINT;
  g_type_class_unref (eclass);
  
  return index;
}

static SfiGlueProc*
bglue_describe_proc (SfiGlueContext *context,
                     const gchar    *proc_name)
{
  GType type = g_type_from_name (proc_name);
  BseProcedureClass *proc;
  SfiGlueProc *p = NULL;
  
  if (!BSE_TYPE_IS_PROCEDURE (type))
    return NULL;
  
  proc = g_type_class_ref (type);
  if (proc->n_out_pspecs < 2)
    {
      guint i;
      
      p = _sfi_glue_proc_new ();
      p->proc_name = g_strdup (g_type_name (type));
      if (proc->n_out_pspecs)
	{
	  GParamSpec *pspec = pspec_to_serializable (proc->out_pspecs[0]);
	  _sfi_glue_proc_add_ret_param (p, pspec);
	  g_param_spec_unref (pspec);
	}
      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  GParamSpec *pspec = pspec_to_serializable (proc->in_pspecs[i]);
	  _sfi_glue_proc_add_param (p, pspec);
	  g_param_spec_unref (pspec);
	}
    }
  g_type_class_unref (proc);
  
  return p;
}

static gchar**
bglue_list_proc_names (SfiGlueContext *context)
{
  BseCategorySeq *cseq = bse_categories_match_typed ("/Proc/""*", BSE_TYPE_PROCEDURE);
  gchar **p;
  guint i;
  
  p = g_new (gchar*, cseq->n_cats + 1);
  for (i = 0; i < cseq->n_cats; i++)
    p[i] = g_strdup (cseq->cats[i]->type);
  p[i] = NULL;
  bse_category_seq_free (cseq);
  
  return p;
}

static gchar**
bglue_list_method_names (SfiGlueContext *context,
                         const gchar    *iface_name)
{
  GType type = g_type_from_name (iface_name);
  BseCategorySeq *cseq;
  gchar **p, *prefix;
  guint i, l, n_procs;
  
  if (!g_type_is_a (type, BSE_TYPE_ITEM))
    return NULL;
  
  prefix = g_strdup_printf ("%s+", g_type_name (type));
  l = strlen (prefix);
  
  cseq = bse_categories_match_typed ("/Method/" "*", BSE_TYPE_PROCEDURE);
  p = g_new (gchar*, cseq->n_cats + 1);
  n_procs = 0;
  for (i = 0; i < cseq->n_cats; i++)
    if (strncmp (cseq->cats[i]->type, prefix, l) == 0)
      p[n_procs++] = g_strdup (cseq->cats[i]->type + l);
  p[n_procs] = NULL;
  bse_category_seq_free (cseq);
  g_free (prefix);
  
  return p;
}

static gchar*
bglue_base_iface (SfiGlueContext *context)
{
  return g_strdup ("BseItem");
}

static gchar**
bglue_iface_children (SfiGlueContext *context,
                      const gchar    *iface_name)
{
  GType type = g_type_from_name (iface_name);
  gchar **childnames = NULL;
  
  if (g_type_is_a (type, BSE_TYPE_ITEM))
    {
      GType *children;
      guint n;
      
      children = g_type_children (type, &n);
      childnames = g_new (gchar*, n + 1);
      childnames[n] = NULL;
      while (n--)
        childnames[n] = g_strdup (g_type_name (children[n]));
      g_free (children);
    }
  return childnames;
}

static BseErrorType
glue_marshal_proc (gpointer           marshal_data,
                   BseProcedureClass *proc,
                   const GValue      *ivalues,
                   GValue            *ovalues)
{
  return proc->execute (proc, ivalues, ovalues);
}

static GValue*
bglue_exec_proc (SfiGlueContext *context,
		 const gchar    *proc_name,
		 SfiSeq         *params)
{
  GValue *retval = NULL;
  GType ptype = bse_procedure_lookup (proc_name);
  
  if (BSE_TYPE_IS_PROCEDURE (ptype) && G_TYPE_IS_DERIVED (ptype))
    {
      BseProcedureClass *proc = g_type_class_ref (ptype);
      GValue *ovalues = g_new0 (GValue, proc->n_out_pspecs);
      GSList *ilist = NULL, *olist = NULL, *clearlist = NULL;
      guint i, sl = sfi_seq_length (params);
      BseErrorType error;
      
      for (i = 0; i < proc->n_in_pspecs; i++)
	{
	  GParamSpec *pspec = proc->in_pspecs[i];
	  if (i < sl)
	    {
	      GValue *sfivalue = sfi_seq_get (params, i);
	      GValue *bsevalue = value_from_serializable (sfivalue, pspec);
	      ilist = g_slist_prepend (ilist, bsevalue ? bsevalue : sfivalue);
	      if (bsevalue)
		clearlist = g_slist_prepend (clearlist, bsevalue);
	    }
	  else
	    {
	      GValue *value = sfi_value_empty ();
	      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	      g_param_value_set_default (pspec, value);
	      ilist = g_slist_prepend (ilist, value);
	      clearlist = g_slist_prepend (clearlist, value);
	    }
	}
      for (i = 0; i < proc->n_out_pspecs; i++)
	{
	  g_value_init (ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
	  olist = g_slist_prepend (olist, ovalues + i);
	}
      
      ilist = g_slist_reverse (ilist);
      olist = g_slist_reverse (olist);
      error = bse_procedure_execvl (proc, ilist, olist, glue_marshal_proc, NULL);
      g_slist_free (ilist);
      g_slist_free (olist);
      for (ilist = clearlist; ilist; ilist = ilist->next)
	sfi_value_free (ilist->data);
      g_slist_free (clearlist);
      
      if (error)
        g_message ("while executing \"%s\": %s\n", proc->name, bse_error_blurb (error));
      
      if (proc->n_out_pspecs)
	retval = value_to_serializable (ovalues + 0);
      for (i = 0; i < proc->n_out_pspecs; i++)
        g_value_unset (ovalues + i);
      g_free (ovalues);
      g_type_class_unref (proc);
    }
  else
    g_message ("failed to execute \"%s\": no such procedure\n", proc_name);
  
  return retval;
}

static gchar*
bglue_proxy_iface (SfiGlueContext *context,
                   SfiProxy        proxy)
{
  BseObject *object = bse_object_from_id (proxy);
  
  if (BSE_IS_ITEM (object))
    return g_strdup (G_OBJECT_TYPE_NAME (object));
  else
    return NULL;
}

static gboolean
bglue_proxy_is_a (SfiGlueContext *context,
		  SfiProxy        proxy,
		  const gchar    *iface)
{
  BseObject *object = bse_object_from_id (proxy);
  GType itype = iface ? g_type_from_name (iface) : 0;

  return object && itype && g_type_is_a (G_OBJECT_TYPE (object), itype);
}

static gchar**
bglue_proxy_list_properties (SfiGlueContext *context,
			     SfiProxy        proxy,
			     const gchar    *first_ancestor,
			     const gchar    *last_ancestor)
{
  BseObject *object = bse_object_from_id (proxy);
  gchar **names = NULL;

  if (BSE_IS_ITEM (object))
    {
      GType first_base_type = first_ancestor ? g_type_from_name (first_ancestor) : 0;
      GType last_base_type = last_ancestor ? g_type_from_name (last_ancestor) : 0;
      guint i, n;
      GParamSpec **pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n);
      gchar **p = g_new (gchar*, n);
      names = p;
      for (i = 0; i < n; i++)
	{
	  GParamSpec *pspec = pspecs[i];
	  
	  if ((!first_base_type || g_type_is_a (pspec->owner_type, first_base_type)) &&
	      (!last_base_type || g_type_is_a (last_base_type, pspec->owner_type)))
	    *p++ = g_strdup (pspec->name);
	}
      g_free (pspecs);
      *p++ = NULL;
      names = g_renew (gchar*, names, p - names);
    }
  return names;
}

static GParamSpec*
bglue_proxy_get_pspec (SfiGlueContext *context,
		       SfiProxy        proxy,
		       const gchar    *prop_name)
{
  BseObject *object = bse_object_from_id (proxy);
  GParamSpec *pspec;
  
  if (!BSE_IS_ITEM (object))
    {
      g_message ("property lookup: no such object (proxy=%lu)", proxy);
      return NULL;
    }
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop_name);
  if (!pspec)
    return NULL;
  
  pspec = pspec_to_serializable (pspec);
  
  return pspec;
}

static SfiSCategory
bglue_proxy_get_pspec_scategory (SfiGlueContext *context,
				 SfiProxy        proxy,
				 const gchar    *prop_name)
{
  GParamSpec *pspec = bglue_proxy_get_pspec (context, proxy, prop_name);
  SfiSCategory scat = 0;
  if (pspec)
    {
      scat = sfi_categorize_pspec (pspec);
      g_param_spec_unref (pspec);
    }
  return scat;
}

static void
bglue_proxy_set_property (SfiGlueContext *context,
			  SfiProxy        proxy,
			  const gchar    *prop,
			  const GValue   *value)
{
  GObject *object = bse_object_from_id (proxy);
  
  if (BSE_IS_OBJECT (object) && prop && G_IS_VALUE (value))
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop);
      
      if (pspec)
	{
	  GValue *pvalue = value_from_serializable (value, pspec);
	  
	  if (pvalue)
	    {
	      g_object_set_property (object, prop, pvalue);
	      sfi_value_free (pvalue);
	    }
	  else
	    g_object_set_property (object, prop, value);
	}
    }
}

static GValue*
bglue_proxy_get_property (SfiGlueContext *context,
			  SfiProxy        proxy,
			  const gchar    *prop)
{
  GObject *object = bse_object_from_id (proxy);
  GValue *rvalue = NULL;
  
  if (BSE_IS_OBJECT (object) && prop)
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), prop);
      
      if (pspec)
	{
	  GValue *value = sfi_value_empty ();
	  g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  g_object_get_property (object, prop, value);
	  rvalue = value_to_serializable (value);
	  sfi_value_free (value);
	}
    }
  return rvalue;
}

static void
proxy_release (BseItem  *item,
	       BContext *bcontext)
{
  SfiProxy proxy = BSE_OBJECT_ID (item);
  BProxy *p = sfi_ustore_lookup (bcontext->proxies, proxy);
  if (p->remote_watch)
    {
      bcontext_queue_release (bcontext, proxy);
      p->remote_watch = FALSE;
    }
  while (p->closures)
    {
      GSList *slist = p->closures;
      GClosure *closure = slist->data;
      BClosure *bclosure = (BClosure*) closure;
      p->closures = slist->next;
      g_slist_free_1 (slist);
      g_closure_invalidate (closure);
      g_signal_handler_disconnect (item, bclosure->handler_id);
      g_closure_unref (closure);
    }
  // FIXME: remove only, if use_count through this context is 0
  g_signal_handler_disconnect (item, p->release_id);
  p->release_id = 0;
  sfi_ustore_remove (bcontext->proxies, proxy);
}

static BProxy*
fetch_proxy (BContext *bcontext,
	     SfiProxy  proxy,
	     BseItem  *item)
{
  BProxy *p;

  p = sfi_ustore_lookup (bcontext->proxies, proxy);
  if (!p && (item->use_count > 0 || item->parent))
    {
      p = g_new0 (BProxy, 1);
      p->release_id = g_signal_connect_data (item, "release", G_CALLBACK (proxy_release), bcontext, NULL, 0);
      p->remote_watch = FALSE;
      sfi_ustore_insert (bcontext->proxies, proxy, p);
    }
  return p;
}

static gboolean
bglue_proxy_watch_release (SfiGlueContext *context,
			   SfiProxy        proxy)
{
  BContext *bcontext = (BContext*) context;
  BseItem *item = bse_object_from_id (proxy);
  BProxy *p;

  if (!BSE_IS_ITEM (item))
    return FALSE;
  p = fetch_proxy (bcontext, proxy, item);
  if (!p)
    return FALSE;
  if (p->remote_watch)
    g_warning ("%s: redundant watch request on proxy (%lu)", bcontext->user, proxy);
  p->remote_watch = TRUE;
  return TRUE;
}

static void
bclosure_marshal (GClosure       *closure,
		  GValue         *return_value,
		  guint           n_param_values,
		  const GValue   *param_values,
		  gpointer        invocation_hint,
		  gpointer        marshal_data)
{
  BClosure *bclosure = (BClosure*) closure;
  BContext *bcontext = closure->data;
  gchar *signal = g_quark_to_string (bclosure->qsignal);
  SfiSeq *args = sfi_seq_new ();
  guint i;

  for (i = 0; i < n_param_values; i++)
    {
      GValue *value = value_to_serializable (param_values + i);
      sfi_seq_append (args, value);
      sfi_value_free (value);
    }
  bcontext_queue_signal (bcontext, signal, args);
  sfi_seq_unref (args);
}

static gboolean
bglue_proxy_notify (SfiGlueContext *context,
		    SfiProxy        proxy,
		    const gchar    *signal,
		    gboolean        enable_notify)
{
  BContext *bcontext = (BContext*) context;
  BseItem *item = bse_object_from_id (proxy);
  GQuark qsignal;
  BProxy *p;
  GClosure *closure;
  BClosure *bclosure;
  GSList *slist, *last = NULL;
  gboolean connected;

  if (!BSE_IS_ITEM (item) || !signal)
    return FALSE;
  p = fetch_proxy (bcontext, proxy, item);
  if (!p)
    return FALSE;

  qsignal = g_quark_from_string (signal);
  for (slist = p->closures; slist; last = slist, slist = last->next)
    {
      bclosure = slist->data;
      if (bclosure->qsignal == qsignal)
	{
	  if (enable_notify)
	    {
	      g_warning ("%s: redundant signal \"%s\" connection on proxy (%lu)", bcontext->user, signal, proxy);
	      return TRUE;
	    }
	  closure = (GClosure*) bclosure;
	  if (last)
	    last->next = slist->next;
	  else
	    p->closures = slist->next;
	  g_slist_free_1 (slist);
	  g_closure_invalidate (closure);
	  g_signal_handler_disconnect (item, bclosure->handler_id);
	  g_closure_unref (closure);
	  return FALSE;
	}
    }
  if (!enable_notify)
    {
      g_warning ("%s: no such signal \"%s\" connection on proxy (%lu)", bcontext->user, signal, proxy);
      return FALSE;
    }

  closure = g_closure_new_simple (sizeof (BClosure), bcontext);
  g_closure_set_marshal (closure, bclosure_marshal);
  bclosure = (BClosure*) closure;
  bclosure->qsignal = qsignal;
  g_closure_ref (closure);
  g_closure_sink (closure);
  bclosure->handler_id = g_signal_connect_closure (item, signal, closure, TRUE);
  if (bclosure->handler_id)
    {
      p->closures = g_slist_prepend (p->closures, closure);
      connected = TRUE;
    }
  else
    {
      connected = FALSE;	/* something failed */
      g_closure_unref (closure);
    }
  return connected;
}

static GValue*
bglue_client_msg (SfiGlueContext *context,
                  const gchar    *msg,
                  GValue         *value)
{
  GValue *retval = NULL;
  
  if (!msg)
    ;
  else if (strcmp (msg, "bse-set-prop") == 0 && SFI_VALUE_HOLDS_SEQ (value))
    {
      SfiSeq *seq = sfi_value_get_seq (value);
      
      if (!seq || seq->n_elements != 3)
        retval = sfi_value_string ("invalid arguments supplied");
      else
        {
          GObject *object = bse_object_from_id (sfi_value_get_proxy (sfi_seq_get (seq, 0)));
          GParamSpec *pspec = NULL;
	  
          if (BSE_IS_ITEM (object))
            pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), sfi_value_get_string (sfi_seq_get (seq, 1)));
          if (pspec)
	    g_object_set_property (object, pspec->name, sfi_seq_get (seq, 2));
          else
            retval = sfi_value_string ("invalid arguments supplied");
        }
    }
  else
    {
      g_message ("unhandled client message: %s\n", msg);
      retval = sfi_value_string ("Unknown client msg");
    }
  
  return retval;
}

/* vim:set ts=8 sts=2 sw=2: */

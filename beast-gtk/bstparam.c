/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstparam.h"

/* xframe rack view support */
#include "bstxframe.h"
#include "bstrackeditor.h"
#include "bstapp.h"
#include "bstdial.h"
#include "bstlogadjustment.h"
#include "bstsequence.h"


/* --- variable --- */
static GQuark quark_null_group = 0;
static GQuark quark_param_choice_values = 0;


/* --- param implementation utils --- */
void
_bst_init_params (void)
{
  g_assert (quark_null_group == 0);

  quark_null_group = g_quark_from_static_string ("bst-param-null-group");
  quark_param_choice_values = g_quark_from_static_string ("bst-param-choice-values");
}


gboolean
bst_param_xframe_check_button (BstParam *bparam,
			       guint     button)
{
  g_return_val_if_fail (bparam != NULL, FALSE);

  if (bparam->binding->rack_item)
    {
      SfiProxy item = bparam->binding->rack_item (bparam);

      if (BSE_IS_ITEM (item))
	{
	  SfiProxy project = bse_item_get_project (item);
	  
	  if (project)
	    {
	      BstApp *app = bst_app_find (project);
	      
	      if (app && app->rack_editor && BST_RACK_EDITOR (app->rack_editor)->rtable->edit_mode)
		{
		  if (button == 1)
		    bst_rack_editor_add_property (BST_RACK_EDITOR (app->rack_editor), item, bparam->pspec->name);
		  return TRUE;
		}
	    }
	}
    }
  return FALSE;
}

gboolean
bst_param_entry_key_press (GtkEntry    *entry,
			   GdkEventKey *event)
{
  GtkEditable *editable = GTK_EDITABLE (entry);
  gboolean intercept = FALSE;

  if (event->state & GDK_MOD1_MASK)
    switch (event->keyval)
      {
      case 'b': /* check gtk_move_backward_word() */
	intercept = gtk_editable_get_position (editable) <= 0;
	break;
      case 'd': /* gtk_delete_forward_word() */
	intercept = TRUE;
	break;
      case 'f': /* check gtk_move_forward_word() */
	intercept = gtk_editable_get_position (editable) >= entry->text_length;
	break;
      default:
	break;
      }
  return intercept;
}

gboolean
bst_param_ensure_focus (GtkWidget *widget)
{
  gtk_widget_grab_focus (widget);
  return FALSE;
}

/* --- BParam functions --- */
static void
bst_param_update_sensitivity (BstParam *bparam)
{
  bparam->writable = (!bparam->readonly &&
		      bparam->editable &&
		      (!bparam->binding->check_writable ||
		       bparam->binding->check_writable (bparam)));

  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    bst_gmask_set_sensitive (bparam->gdata.gmask, bparam->writable);
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    gtk_widget_set_sensitive (bparam->gdata.widget, bparam->writable);
}

BstParam*
bst_param_alloc (BstParamImpl *impl,
		 GParamSpec   *pspec)
{
  BstParam *bparam;
  GType itype, vtype;

  g_return_val_if_fail (impl != NULL, NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (!impl->create_gmask ^ !impl->create_widget, NULL);

  bparam = g_new0 (BstParam, 1);
  bparam->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  bparam->impl = impl;
  vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  itype = sfi_category_type (bparam->impl->scat);
  if (!itype)
    itype = vtype;
  g_value_init (&bparam->value, itype);
  bparam->column = 0;
  bparam->readonly = (!(bparam->impl->flags & BST_PARAM_EDITABLE) ||
		      !(pspec->flags & G_PARAM_WRITABLE) ||
		      sfi_pspec_test_hint (pspec, SFI_PARAM_HINT_RDONLY));
  bparam->writable = FALSE;
  bparam->editable = TRUE;
  bparam->updating = FALSE;
  bparam->needs_transform = !g_value_type_compatible (vtype, itype);
  if (bparam->impl->flags & BST_PARAM_EDITABLE)
    bparam->needs_transform |= !g_value_type_compatible (itype, vtype);
  bparam->gdata.widget = NULL;
  return bparam;
}

void
bst_param_update (BstParam *bparam)
{
  GtkWidget *action = NULL;
  gboolean updating;
  
  g_return_if_fail (bparam != NULL);
  
  updating = bparam->updating;
  bparam->updating = TRUE;
  
  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    action = bst_gmask_get_action (bparam->gdata.gmask);
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    action = bparam->gdata.widget;
  
  if (bparam->needs_transform)
    {
      GValue tvalue = { 0, };
      g_value_init (&tvalue, G_PARAM_SPEC_VALUE_TYPE (bparam->pspec));
      bparam->binding->get_value (bparam, &tvalue);
      g_value_transform (&tvalue, &bparam->value);
      g_value_unset (&tvalue);
    }
  else
    bparam->binding->get_value (bparam, &bparam->value);
  
  if (action)
    bparam->impl->update (bparam, action);
  
  bparam->writable = (!bparam->readonly &&
		      bparam->editable &&
		      (!bparam->binding->check_writable ||
		       bparam->binding->check_writable (bparam)));
  bst_param_update_sensitivity (bparam);
  
  bparam->updating = updating;
}

void
bst_param_apply_value (BstParam *bparam)
{
  g_return_if_fail (bparam != NULL);

  if (bparam->updating)
    {
      g_warning ("not applying value from parameter \"%s\" currently in update mode",
		 bparam->impl->name);
      return;
    }
  if (bparam->needs_transform)
    {
      GValue tvalue = { 0, };
      g_value_init (&tvalue, G_PARAM_SPEC_VALUE_TYPE (bparam->pspec));
      g_value_transform (&bparam->value, &tvalue);
      g_param_value_validate (bparam->pspec, &tvalue);
      bparam->binding->set_value (bparam, &tvalue);
      g_value_unset (&tvalue);
    }
  else
    bparam->binding->set_value (bparam, &bparam->value);
}

void
bst_param_set_editable (BstParam *bparam,
			gboolean  editable)
{
  g_return_if_fail (bparam != NULL);

  bparam->editable = editable != FALSE;
  bst_param_update_sensitivity (bparam);
}

static GtkWidget*
bparam_make_container (GtkWidget *parent,
		       GQuark     quark_group)
{
  GtkWidget *container;

  container = bst_container_get_named_child (parent, quark_group ? quark_group : quark_null_group);
  if (!container || !GTK_IS_CONTAINER (container))
    {
      GtkWidget *any;
      container = bst_gmask_container_create (GXK_TOOLTIPS, quark_group ? 5 : 0, FALSE);
      if (quark_group)
	any = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "label", g_quark_to_string (quark_group),
			      "child", container,
			      NULL);
      else
	any = container;
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), any, FALSE, TRUE, 0);
      else if (GTK_IS_WRAP_BOX (parent))
	gtk_container_add_with_properties (GTK_CONTAINER (parent), any,
					   "hexpand", TRUE, "hfill", TRUE,
					   "vexpand", FALSE, "vfill", TRUE,
					   NULL);
      else
	gtk_container_add (GTK_CONTAINER (parent), any);
      bst_container_set_named_child (parent, quark_group ? quark_group : quark_null_group, container);
    }
  return container;
}

static gchar*
bst_param_create_tooltip (BstParam *bparam)
{
  gchar *tooltip = g_param_spec_get_blurb (bparam->pspec);
  if (!BST_DVL_HINTS)
    tooltip = g_strdup (tooltip);
  else if (tooltip)
    tooltip = g_strdup_printf ("(%s): %s", g_param_spec_get_name (bparam->pspec), tooltip);
  else
    tooltip = g_strdup_printf ("(%s)", g_param_spec_get_name (bparam->pspec));
  return tooltip;
}

void
bst_param_pack_property (BstParam       *bparam,
			 GtkWidget      *parent)
{
  const gchar *group;
  gchar *tooltip;

  g_return_if_fail (bparam != NULL);
  g_return_if_fail (GTK_IS_CONTAINER (parent));
  g_return_if_fail (bparam->gdata.gmask == NULL);
  g_return_if_fail (BST_PARAM_IS_GMASK (bparam));

  bparam->updating = TRUE;
  group = sfi_pspec_get_group (bparam->pspec);
  parent = bparam_make_container (parent, group ? g_quark_from_string (group) : 0);
  tooltip = bst_param_create_tooltip (bparam);
  bparam->gdata.gmask = bparam->impl->create_gmask (bparam, tooltip, parent);
  g_free (tooltip);
  bst_gmask_ref (bparam->gdata.gmask);
  bst_gmask_set_column (bparam->gdata.gmask, bparam->column);
  bst_gmask_pack (bparam->gdata.gmask);
  bparam->updating = FALSE;
  bst_param_update_sensitivity (bparam); /* bst_param_update (bparam); */
}

GtkWidget*
bst_param_rack_widget (BstParam *bparam)
{
  gchar *tooltip;

  g_return_val_if_fail (bparam != NULL, NULL);
  g_return_val_if_fail (bparam->gdata.widget == NULL, NULL);
  g_return_val_if_fail (!BST_PARAM_IS_GMASK (bparam), NULL);

  bparam->updating = TRUE;
  tooltip = bst_param_create_tooltip (bparam);
  bparam->gdata.widget = bparam->impl->create_widget (bparam, tooltip);
  g_free (tooltip);
  g_object_ref (bparam->gdata.widget);
  bparam->updating = FALSE;
  bst_param_update_sensitivity (bparam); /* bst_param_update (bparam); */
  return bparam->gdata.widget;
}

void
bst_param_destroy (BstParam *bparam)
{
  g_return_if_fail (bparam != NULL);

  bparam->binding->destroy (bparam);
  bparam->binding = NULL;
  if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
    {
      bst_gmask_destroy (bparam->gdata.gmask);
      bst_gmask_unref (bparam->gdata.gmask);
    }
  else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
    {
      gtk_widget_destroy (bparam->gdata.widget);
      g_object_unref (bparam->gdata.widget);
    }
  g_param_spec_unref (bparam->pspec);
  g_value_unset (&bparam->value);
  g_free (bparam);
}


/* --- bindings --- */
static void
proxy_binding_set_value (BstParam       *bparam,
			 const GValue   *value)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    sfi_glue_proxy_set_property (bparam->mdata[0].v_long, bparam->pspec->name, value);
}

static void
proxy_binding_get_value (BstParam       *bparam,
			 GValue         *value)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      const GValue *cvalue = sfi_glue_proxy_get_property (bparam->mdata[0].v_long, bparam->pspec->name);
      g_value_transform (cvalue, value);
    }
  else
    g_value_reset (value);
}

static SfiProxy
proxy_binding_rack_item (BstParam *bparam)
{
  return bparam->mdata[0].v_long;
}

static void
proxy_binding_weakref (gpointer data,
		       SfiProxy junk)
{
  BstParam *bparam = data;
  bparam->mdata[0].v_long = 0;
  bparam->mdata[1].v_long = 0;	/* already disconnected */
  bparam->binding = NULL;
}

static void
proxy_binding_destroy (BstParam *bparam)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      sfi_glue_signal_disconnect (proxy, bparam->mdata[1].v_long);
      sfi_glue_proxy_weak_unref (proxy, proxy_binding_weakref, bparam);
      bparam->mdata[0].v_long = 0;
      bparam->mdata[1].v_long = 0;
      bparam->binding = NULL;
    }
}

static BseProxySeq*
proxy_binding_list_proxies (BstParam *bparam)
{
  SfiProxy proxy = bparam->mdata[0].v_long;
  if (proxy)
    {
      BseProxySeq *pseq = bse_item_list_proxies (proxy, bparam->pspec->name);
      if (pseq)	/* need to return "newly allocated" proxy list */
	return bse_proxy_seq_copy_shallow (pseq);
    }
  return NULL;
}

static BstParamBinding bst_proxy_binding = {
  proxy_binding_set_value,
  proxy_binding_get_value,
  proxy_binding_destroy,
  NULL,	/* check_writable */
  proxy_binding_rack_item,
  proxy_binding_list_proxies,
};

BstParam*
bst_proxy_param_create (GParamSpec  *pspec,
			SfiProxy     proxy,
			const gchar *view_name)
{
  BstParamImpl *impl;
  BstParam *bparam;

  g_return_val_if_fail (BSE_IS_ITEM (proxy), NULL);

  impl = bst_param_lookup_impl (pspec, FALSE, view_name, &bst_proxy_binding);
  if (!impl)
    impl = bst_param_lookup_impl (pspec, FALSE, NULL, &bst_proxy_binding);
  bparam = bst_param_alloc (impl, pspec);
  bparam->binding = &bst_proxy_binding;
  bparam->mdata[0].v_long = 0;
  bst_proxy_param_set_proxy (bparam, proxy);
  return bparam;
}

void
bst_proxy_param_set_proxy (BstParam *bparam,
			   SfiProxy  proxy)
{
  g_return_if_fail (bparam != NULL);
  g_return_if_fail (bparam->binding == &bst_proxy_binding);

  proxy_binding_destroy (bparam);
  bparam->binding = &bst_proxy_binding;
  bparam->mdata[0].v_long = proxy;
  if (proxy)
    {
      gchar *sig = g_strconcat ("property-notify::", bparam->pspec->name, NULL);
      bparam->mdata[1].v_long = sfi_glue_signal_connect_swapped (proxy, sig, bst_param_update, bparam);
      g_free (sig);
      sfi_glue_proxy_weak_ref (proxy, proxy_binding_weakref, bparam);
    }
}


/* --- param and rack widget implementations --- */
#include "bstparam-label.c"
#include "bstparam-toggle.c"
#include "bstparam-spinner.c"
#include "bstparam-entry.c"
#include "bstparam-note-sequence.c"
#include "bstparam-choice.c"
#include "bstparam-strnum.c"
#include "bstparam-note-spinner.c"
#include "bstparam-proxy.c"

static BstParamImpl *bst_param_impls[] = {
  &param_pspec,
  &param_toggle,
  &param_spinner_int,
  &param_spinner_num,
  &param_spinner_real,
  &param_entry,
  &param_note_sequence,
  &param_choice,
  &param_note,
  &param_time,
  &param_note_spinner,
  &param_proxy,
};

static BstParamImpl *bst_rack_impls[] = {
  &rack_pspec,
  &rack_toggle,
  &rack_spinner_int,
  &rack_spinner_num,
  &rack_spinner_real,
  &rack_entry,
  &rack_note_sequence,
  &rack_choice,
  &rack_note,
  &rack_time,
  &rack_note_spinner,
  &rack_proxy,
};

static guint
bst_param_rate_impl (BstParamImpl    *impl,
		     GParamSpec      *pspec,
		     BstParamBinding *binding)
{
  gboolean can_fetch, does_match, type_specific, type_mismatch;
  gboolean good_update = FALSE, good_fetch = FALSE, scat_specific = FALSE;
  GType vtype, itype;
  guint rating = 0;

  g_return_val_if_fail (impl != NULL, 0);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), 0);

  vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  itype = impl->scat ? sfi_category_type (impl->scat) : 0;
  type_specific = itype != 0;

  can_fetch = (impl->flags & BST_PARAM_EDITABLE) != 0;
  if (impl->scat)
    {
      if (impl->scat & ~SFI_SCAT_TYPE_MASK)	/* be strict for non-fundamental scats */
	{
	  type_mismatch = impl->scat != sfi_categorize_pspec (pspec);
	  scat_specific = TRUE;
	}
      else
	type_mismatch = FALSE;
      type_mismatch |= !g_value_type_transformable (vtype, itype);		/* read value */
      type_mismatch |= can_fetch && !g_value_type_transformable (itype, vtype);	/* write value */
    }
  else
    type_mismatch = FALSE;
  if (impl->flags & BST_PARAM_PROXY_LIST)
    type_mismatch |= !binding || !binding->list_proxies;

  does_match = !type_mismatch && (!impl->hints || sfi_pspec_test_all_hints (pspec, impl->hints));
  if (!does_match)
    return 0;		/* mismatch */

  if (itype)
    {
      good_update = g_type_is_a (vtype, itype);
      good_fetch = can_fetch && g_type_is_a (itype, vtype);
    }

  rating |= (good_fetch && good_update);
  rating <<= 1;
  rating |= can_fetch;
  rating <<= 1;
  rating |= good_update;
  rating <<= 1;
  rating |= type_specific;
  rating <<= 1;
  rating |= scat_specific;
  rating <<= 8;
  rating += 128 + impl->rating; /* impl->rating is signed, 8bit */

  return rating;
}

BstParamImpl*
bst_param_lookup_impl (GParamSpec      *pspec,
		       gboolean         rack_widget,
		       const gchar     *name,
		       BstParamBinding *binding)
{
  BstParamImpl **impls = rack_widget ? bst_rack_impls : bst_param_impls;
  guint i, n = rack_widget ? G_N_ELEMENTS (bst_rack_impls) : G_N_ELEMENTS (bst_param_impls);
  BstParamImpl *best = NULL;
  guint rating = 0; /* threshold for mismatch */

  for (i = 0; i < n; i++)
    if (!name || !strcmp (impls[i]->name, name))
      {
	guint r = bst_param_rate_impl (impls[i], pspec, binding);
	if (r > rating) /* only notice improvements */
	  {
	    best = impls[i];
	    rating = r;
	  }
      }
  /* if !name, best is != NULL */
  return best;
}

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


/* --- macros --- */
#define	FIGURE_SENSITIVE(bparam)	(((bparam)->writable && (bparam)->editable) || (bparam)->force_sensitive)


/* --- variable --- */
static GQuark quark_null_group = 0;


/* --- param implementation utils --- */
void
_bst_init_params (void)
{
  g_assert (quark_null_group == 0);

  quark_null_group = g_quark_from_static_string ("bst-param-null-group");
}

BstParam*
bst_param_alloc (BstParamImpl *impl,
		 GParamSpec   *pspec)
{
  BstParam *bparam;

  g_return_val_if_fail (impl != NULL, NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (!impl->create_gmask ^ !impl->create_widget, NULL);

  bparam = g_new0 (BstParam, 1);
  bparam->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  g_value_init (&bparam->value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  bparam->impl = impl;
  bparam->editable = TRUE;
  bparam->force_sensitive = (bparam->impl->flags & BST_PARAM_EDITABLE) == 0;
  bparam->readonly = (!(bparam->impl->flags & BST_PARAM_EDITABLE) ||
		      !(pspec->flags & G_PARAM_WRITABLE) ||
		      sfi_pspec_test_hint (pspec, SFI_PARAM_HINT_RDONLY));
  return bparam;
}


/* --- BParam functions --- */
void
bst_param_update (BstParam *bparam)
{
  GtkWidget *action = NULL;
  gboolean writable;

  g_return_if_fail (bparam != NULL);

  writable = (!bparam->readonly &&
	      (!bparam->binding->check_writable ||
	       bparam->binding->check_writable (bparam)));
  if (writable != bparam->writable)
    {
      bparam->writable = writable;
      if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
	{
	  action = bst_gmask_get_action (bparam->gdata.gmask);
	  bst_gmask_set_sensitive (bparam->gdata.gmask, FIGURE_SENSITIVE (bparam));
	}
      else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
	{
	  action = bparam->gdata.widget;
	  gtk_widget_set_sensitive (bparam->gdata.widget, FIGURE_SENSITIVE (bparam));
	}
    }
  else
    {
      if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
	action = bst_gmask_get_action (bparam->gdata.gmask);
      else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
	action = bparam->gdata.widget;
    }
  if (action)
    bparam->impl->update (bparam, action);
}

void
bst_param_set_editable (BstParam *bparam,
			gboolean  editable)
{
  g_return_if_fail (bparam != NULL);

  editable = editable != FALSE;
  if (editable != bparam->editable)
    {
      bparam->editable = editable;
      if (BST_PARAM_IS_GMASK (bparam) && bparam->gdata.gmask)
	bst_gmask_set_sensitive (bparam->gdata.gmask, FIGURE_SENSITIVE (bparam));
      else if (!BST_PARAM_IS_GMASK (bparam) && bparam->gdata.widget)
	gtk_widget_set_sensitive (bparam->gdata.widget, FIGURE_SENSITIVE (bparam));
    }
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

void
bst_param_pack_property (BstParam       *bparam,
			 GtkWidget      *parent)
{
  const gchar *group;

  g_return_if_fail (bparam != NULL);
  g_return_if_fail (GTK_IS_CONTAINER (parent));
  g_return_if_fail (bparam->gdata.gmask == NULL);
  g_return_if_fail (BST_PARAM_IS_GMASK (bparam));

  group = sfi_pspec_get_group (bparam->pspec);
  parent = bparam_make_container (parent, group ? g_quark_from_string (group) : 0);
  bparam->gdata.gmask = bparam->impl->create_gmask (bparam, parent);
  bst_gmask_ref (bparam->gdata.gmask);
  bst_gmask_set_column (bparam->gdata.gmask, bparam->column);
  bst_gmask_pack (bparam->gdata.gmask);
}

GtkWidget*
bst_param_rack_widget (BstParam *bparam)
{
  g_return_val_if_fail (bparam != NULL, NULL);
  g_return_val_if_fail (bparam->gdata.widget == NULL, NULL);
  g_return_val_if_fail (!BST_PARAM_IS_GMASK (bparam), NULL);

  bparam->gdata.widget = bparam->impl->create_widget (bparam);
  g_object_ref (bparam->gdata.widget);
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

static BstParamBinding bst_proxy_binding = {
  proxy_binding_set_value,
  proxy_binding_get_value,
  proxy_binding_destroy,
};

BstParam*
bst_proxy_param_create (GParamSpec  *pspec,
			SfiProxy     proxy,
			const gchar *view_name)
{
  BstParamImpl *impl;
  BstParam *bparam;

  g_return_val_if_fail (BSE_IS_ITEM (proxy), NULL);

  impl = bst_param_lookup_impl (pspec, FALSE, view_name);
  if (!impl)
    impl = bst_param_lookup_impl (pspec, FALSE, NULL);
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
      gchar *sig = g_strconcat ("notify::", bparam->pspec->name, NULL);
      bparam->mdata[1].v_long = sfi_glue_signal_connect_swapped (proxy, sig, bst_param_update, bparam);
      g_free (sig);
      sfi_glue_proxy_weak_ref (proxy, proxy_binding_weakref, bparam);
    }
}


/* --- param and rack widget implementations --- */
#include "bstparam-label.c"

static BstParamImpl *bst_param_impls[] = {
  &param_pspec,
};

static BstParamImpl *bst_rack_impls[] = {
  &rack_pspec,
};

static gint
bst_param_rate_impl (BstParamImpl *impl,
		     GParamSpec   *pspec)
{
  gboolean can_fetch, can_update, does_match, type_specific;
  gboolean good_update = FALSE, good_fetch = FALSE, fetch_mismatch = FALSE;
  GType vtype, itype;
  gint rating = 0;

  g_return_val_if_fail (impl != NULL, G_MININT);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), G_MININT);

  vtype = G_PARAM_SPEC_VALUE_TYPE (pspec);
  itype = impl->scat ? sfi_category_type (impl->scat) : 0;
  type_specific = itype != 0;

  can_fetch = (impl->flags & BST_PARAM_EDITABLE) != 0;
  if (impl->scat)
    {
      can_update = g_value_type_transformable (vtype, itype);
      fetch_mismatch = !g_value_type_transformable (itype, vtype);
    }
  else
    can_update = TRUE;

  does_match = can_update && !fetch_mismatch;
  /* could call check() here */
  if (!does_match)
    return G_MININT;

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
  rating <<= 8;
  rating += impl->rating;

  return rating;
}

BstParamImpl*
bst_param_lookup_impl (GParamSpec     *pspec,
		       gboolean        rack_widget,
		       const gchar    *name)
{
  BstParamImpl **impls = rack_widget ? bst_rack_impls : bst_param_impls;
  guint i, n = rack_widget ? G_N_ELEMENTS (bst_rack_impls) : G_N_ELEMENTS (bst_param_impls);
  BstParamImpl *best = NULL;
  gint rating = G_MININT; /* threshold for mismatch */

  if (name)
    for (i = 0; i < n; i++)
      if (!strcmp (impls[i]->name, name))
	return bst_param_rate_impl (impls[i], pspec) > rating ? impls[i] : NULL;
  for (i = 0; i < n; i++)
    {
      guint r = bst_param_rate_impl (impls[i], pspec);
      if (r > rating) /* only notice improvements */
	{
	  best = impls[i];
	  rating = r;
	}
    }
  return best;
}

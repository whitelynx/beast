/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstpreferences.h"

#include "bstgconfig.h"
#include "bstparam.h"


/* --- prototypes --- */
static void	  bst_preferences_class_init		(BstPreferencesClass	*klass);
static void	  bst_preferences_init			(BstPreferences		*prefs);
static void	  bst_preferences_destroy		(GtkObject		*object);
static GtkWidget* bst_preferences_build_rec_editor	(SfiRec			*rec,
							 SfiRecFields		 fields,
							 SfiRing	       **bparam_list);


/* --- static variables --- */
static gpointer             parent_class = NULL;


/* --- functions --- */
GtkType
bst_preferences_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstPreferencesClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_preferences_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstPreferences),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_preferences_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX, "BstPreferences", &type_info, 0);
    }
  return type;
}

static void
bst_preferences_class_init (BstPreferencesClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = bst_preferences_destroy;
}

static void
bst_preferences_init (BstPreferences *prefs)
{
  GParamSpec *pspec;
  GtkWidget *pchild;

  prefs->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
				  "visible", TRUE,
				  "parent", prefs,
				  "tab_pos", GTK_POS_TOP,
				  "scrollable", FALSE,
				  "can_focus", TRUE,
				  "border_width", 5,
				  NULL);
  gxk_nullify_on_destroy (prefs->notebook, &prefs->notebook);
  g_object_connect (prefs->notebook,
		    "signal_after::switch-page", gtk_widget_viewable_changed, NULL,
		    NULL);

  pspec = bst_gconfig_pspec ();
  prefs->bstrec = bst_gconfig_to_rec (bst_global_config);
  pchild = bst_preferences_build_rec_editor (prefs->bstrec, sfi_pspec_get_rec_fields (pspec), &prefs->bstparams);
  gxk_notebook_append (prefs->notebook, pchild, "BEAST");
}

static void
bst_preferences_destroy (GtkObject *object)
{
  BstPreferences *prefs = BST_PREFERENCES (object);

  sfi_ring_free (prefs->bstparams);
  prefs->bstparams = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GtkWidget*
bst_preferences_build_rec_editor (SfiRec      *rec,
				  SfiRecFields fields,
				  SfiRing    **bparam_list)
{
  GtkWidget *parent;
  SfiRing *ring, *bparams = NULL;
  guint i;

  g_return_val_if_fail (rec != NULL, NULL);

  parent = g_object_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "border_width", 5,
			 NULL);
  for (i = 0; i < fields.n_fields; i++)
    {
      GParamSpec *pspec = fields.fields[i];
      if (sfi_pspec_test_hint (pspec, SFI_PARAM_SERVE_GUI) && (pspec->flags & G_PARAM_READABLE))
	{
	  BstParam *bparam = bst_rec_param_create (pspec, rec, NULL);
	  bst_param_pack_property (bparam, parent);
	  bparams = sfi_ring_append (bparams, bparam);
	}
    }
  for (ring = bparams; ring; ring = sfi_ring_walk (ring, bparams))
    bst_param_update (ring->data);
  if (bparam_list)
    *bparam_list = bparams;
  else
    sfi_ring_free (bparams);
  return parent;
}

static void
bst_preferences_update (BstPreferences *prefs)
{
  SfiRing *ring;
  for (ring = prefs->bstparams; ring; ring = sfi_ring_walk (ring, prefs->bstparams))
    bst_param_update (ring->data);
}

void
bst_preferences_revert (BstPreferences *prefs)
{
  SfiRec *rec, *crec;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  rec = bst_gconfig_to_rec (bst_global_config);
  crec = sfi_rec_copy_deep (rec);
  sfi_rec_unref (rec);
  sfi_rec_swap_fields (prefs->bstrec, crec);
  sfi_rec_unref (crec);
  bst_preferences_update (prefs);
}

void
bst_preferences_apply (BstPreferences *prefs)
{
  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  bst_gconfig_apply (prefs->bstrec);
  bst_preferences_revert (prefs);
}

void
bst_preferences_default_revert (BstPreferences *prefs)
{
  SfiRec *rec;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  rec = sfi_rec_new ();
  sfi_rec_validate (rec, sfi_pspec_get_rec_fields (bst_gconfig_pspec ()));
  sfi_rec_swap_fields (prefs->bstrec, rec);
  sfi_rec_unref (rec);
  bst_preferences_update (prefs);
}

void
bst_preferences_save (BstPreferences *prefs)
{
  BseErrorType error = 0;
  gchar *file_name;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));

  file_name = BST_STRDUP_RC_FILE ();
  // error = bst_rc_dump (file_name, prefs->gconf);
  if (error)
    g_warning ("error saving rc-file \"%s\": %s", file_name, bse_error_blurb (error));
  g_free (file_name);
}

void
bst_preferences_create_buttons (BstPreferences *prefs,
				GxkDialog      *dialog)
{
  GtkWidget *widget;

  g_return_if_fail (BST_IS_PREFERENCES (prefs));
  g_return_if_fail (GXK_IS_DIALOG (dialog));
  g_return_if_fail (prefs->apply == NULL);

  /* Apply
   */
  prefs->apply = g_object_connect (gxk_dialog_default_action (dialog, BST_STOCK_APPLY, NULL, NULL),
				   "swapped_signal::clicked", bst_preferences_apply, prefs,
				   "swapped_signal::clicked", bst_preferences_save, prefs,
				   "swapped_signal::destroy", g_nullify_pointer, &prefs->apply,
				   NULL);
  gtk_tooltips_set_tip (BST_TOOLTIPS, prefs->apply,
			"Apply and save the preference values. Some values may only take effect after "
			"restart while others can be locked against modifcation during "
			"playback.",
			NULL);

  /* Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_REVERT, bst_preferences_revert, prefs);
  gtk_tooltips_set_tip (BST_TOOLTIPS, widget,
			"Revert to the currently active values.",
			NULL);

  /* Default Revert
   */
  widget = gxk_dialog_action_swapped (dialog, BST_STOCK_DEFAULT_REVERT, bst_preferences_default_revert, prefs);
  gtk_tooltips_set_tip (BST_TOOLTIPS, widget,
			"Revert to hardcoded default values (factory settings).",
			NULL);

  /* Close
   */
  widget = gxk_dialog_action (dialog, BST_STOCK_CLOSE, gxk_toplevel_delete, NULL);
  gtk_tooltips_set_tip (BST_TOOLTIPS, widget,
			"Discard changes and close dialog.",
			NULL);
}


#if 0
/* --- rc file --- */
#include <fcntl.h>
#include <errno.h>
#include "../PKG_config.h"
static void
ifactory_print_func (gpointer     user_data,
		     const gchar *str)
{
  BseStorage *storage = user_data;

  bse_storage_break (storage);
  // bse_storage_indent (storage);
  bse_storage_puts (storage, str);
  if (str[0] == ';')
    bse_storage_needs_break (storage);
}

static void
accel_map_print (gpointer        data,
		 const gchar    *accel_path,
		 guint           accel_key,
		 guint           accel_mods,
		 gboolean        changed)
{
  GString *gstring = g_string_new (changed ? NULL : "; ");
  BseStorage *storage = data;
  gchar *tmp, *name;

  g_string_append (gstring, "(gtk_accel_path \"");

  tmp = g_strescape (accel_path, NULL);
  g_string_append (gstring, tmp);
  g_free (tmp);

  g_string_append (gstring, "\" \"");

  name = gtk_accelerator_name (accel_key, accel_mods);
  tmp = g_strescape (name, NULL);
  g_free (name);
  g_string_append (gstring, tmp);
  g_free (tmp);

  g_string_append (gstring, "\")");

  bse_storage_puts (storage, gstring->str);
  bse_storage_break (storage);

  g_string_free (gstring, TRUE);
}

BseErrorType
bst_rc_dump (const gchar *file_name,
	     BseGConfig  *gconf)
{
  BseStorage *storage;
  gint fd;

  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_GCONFIG (gconf), BSE_ERROR_INTERNAL);

  fd = open (file_name,
	     O_WRONLY | O_CREAT | O_TRUNC, /* O_EXCL, */
	     0666);

  if (fd < 0)
    return (errno == EEXIST ? BSE_ERROR_FILE_EXISTS : BSE_ERROR_FILE_IO);

  storage = bse_storage_new ();
  bse_storage_prepare_write (storage, TRUE);

  /* put blurb
   */
  bse_storage_puts (storage, "; rc-file for BEAST v" BST_VERSION);

  bse_storage_break (storage);
  
  /* store BseGlobals
   */
  bse_storage_break (storage);
  bse_storage_puts (storage, "; BseGlobals dump");
  bse_storage_break (storage);
  bse_storage_puts (storage, "(preferences");
  bse_storage_push_level (storage);
  bse_object_store (BSE_OBJECT (gconf), storage);
  bse_storage_pop_level (storage);

  bse_storage_break (storage);

  /* store item factory paths
   */
  bse_storage_break (storage);
  bse_storage_puts (storage, "; GtkItemFactory path dump");
  bse_storage_break (storage);
  bse_storage_puts (storage, "(menu-accelerators");
  bse_storage_push_level (storage);
  bse_storage_break (storage);
  gtk_accel_map_foreach (storage, accel_map_print);
  bse_storage_pop_level (storage);
  bse_storage_handle_break (storage);
  bse_storage_putc (storage, ')');
  
  /* flush stuff to rc file
   */
  bse_storage_flush_fd (storage, fd);
  bse_storage_destroy (storage);

  return close (fd) < 0 ? BSE_ERROR_FILE_IO : BSE_ERROR_NONE;
}

BseErrorType
bst_rc_parse (const gchar *file_name,
	      BseGConfig  *gconf)
{
  BseStorage *storage;
  GScanner *scanner;
  GTokenType expected_token = G_TOKEN_NONE;
  BseErrorType error;

  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_GCONFIG (gconf), BSE_ERROR_INTERNAL);
  
  storage = bse_storage_new ();
  error = bse_storage_input_file (storage, file_name);
  if (error)
    {
      bse_storage_destroy (storage);
      return error;
    }
  scanner = storage->scanner;

  while (!bse_storage_input_eof (storage) && expected_token == G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);

      if (scanner->token == G_TOKEN_EOF)
	break;
      else if (scanner->token == '(' && g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
	{
	  g_scanner_get_next_token (scanner);
	  if (bse_string_equals ("preferences", scanner->value.v_identifier))
	    expected_token = bse_object_restore (BSE_OBJECT (gconf), storage);
	  else if (bse_string_equals ("menu-accelerators", scanner->value.v_identifier))
	    {
	      gtk_accel_map_load_scanner (scanner);

	      if (g_scanner_get_next_token (scanner) != ')')
		expected_token = ')';
	    }
	  else
	    expected_token = G_TOKEN_IDENTIFIER;
	}
      else
	expected_token = G_TOKEN_EOF;
    }

  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  error = scanner->parse_errors >= scanner->max_parse_errors ? BSE_ERROR_PARSE_ERROR : BSE_ERROR_NONE;

  bse_storage_destroy (storage);

  return error;
}
#endif

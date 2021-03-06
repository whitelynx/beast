/* BEAST - Better Audio System
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstfiledialog.h"
#include "bstmenus.h"
#include "bsttreestores.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


/* --- prototypes --- */
static void	bst_file_dialog_finalize	(GObject		*object);
static void	bst_file_dialog_activate	(BstFileDialog		*self);


/* --- functions --- */
G_DEFINE_TYPE (BstFileDialog, bst_file_dialog, GXK_TYPE_DIALOG);

static void
bst_file_dialog_class_init (BstFileDialogClass	*class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  gobject_class->finalize = bst_file_dialog_finalize;
}

static void
tree_viewable_changed (BstFileDialog *self)
{
  gboolean viewable = gxk_widget_viewable (GTK_WIDGET (self->tview));
  gboolean tvisible = GTK_WIDGET_VISIBLE (gtk_widget_get_toplevel (GTK_WIDGET (self->tview)));
  
  if (viewable && !self->using_file_store)
    {
      self->using_file_store = TRUE;
      bst_file_store_update_list (self->file_store, self->search_path, self->search_filter);
    }
  else if (!tvisible && self->using_file_store)
    {
      self->using_file_store = FALSE;
      bst_file_store_forget_list (self->file_store);
    }
}

static void
bst_file_dialog_init (BstFileDialog *self)
{
  GtkTreeSelection *tsel;
  GtkTreeModel *smodel;
  GtkWidget *bbox, *vbox;
  GtkWidget *main_box = GXK_DIALOG (self)->vbox;
  
  self->ignore_activate = TRUE;
  
  /* configure self */
  g_object_set (self,
		"flags", (GXK_DIALOG_HIDE_ON_DELETE |
                          GXK_DIALOG_PRESERVE_STATE |
			  GXK_DIALOG_POPUP_POS |
			  GXK_DIALOG_MODAL),
		NULL);
  gxk_dialog_set_sizes (GXK_DIALOG (self), -1, -1, 500, 450);
  g_object_set (main_box,
		"homogeneous", FALSE,
		"spacing", 0,
		"border_width", 0,
		NULL);
  
  /* notebook */
  self->notebook = g_object_new (GXK_TYPE_NOTEBOOK,
				 "visible", TRUE,
				 "show_border", TRUE,
				 "show_tabs", TRUE,
				 "scrollable", FALSE,
				 "tab_border", 0,
				 "enable_popup", TRUE,
				 "tab_pos", GTK_POS_TOP,
				 "border_width", 5,
				 "parent", main_box,
				 "enable_popup", FALSE,
				 NULL);
  
  
  /* setup file selection widgets and add to notebook */
  self->fs = (GtkFileSelection*) gtk_file_selection_new ("");
  self->fpage = gxk_file_selection_split (self->fs, &bbox);
  g_object_ref (self->fpage);
  gtk_container_remove (GTK_CONTAINER (self->fpage->parent), self->fpage);
  gxk_notebook_append (GTK_NOTEBOOK (self->notebook), self->fpage, "File Selection", TRUE);
  g_object_unref (self->fpage);
  
  /* sample selection tree */
  self->spage = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			      "visible", TRUE,
			      "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			      "vscrollbar_policy", GTK_POLICY_ALWAYS,
			      "border_width", 5,
			      "shadow_type", GTK_SHADOW_IN,
			      NULL);
  gxk_notebook_append (GTK_NOTEBOOK (self->notebook), self->spage, "Sample Selection", TRUE);
  self->file_store = bst_file_store_create ();
  smodel = gtk_tree_model_sort_new_with_model (self->file_store);
  self->tview = g_object_new (GTK_TYPE_TREE_VIEW,
			      "visible", TRUE,
			      "can_focus", TRUE,
			      "model", smodel,
			      "rules_hint", TRUE,
			      "parent", self->spage,
			      "search_column", BST_FILE_STORE_COL_BASE_NAME,
			      NULL);
  g_object_unref (smodel);
  g_object_connect (self->tview,
		    "swapped_signal::viewable-changed", tree_viewable_changed, self,
		    NULL);
  tsel = gtk_tree_view_get_selection (self->tview);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, smodel);
  g_object_connect (self->tview, "swapped_object_signal::row_activated", gtk_button_clicked, self->fs->ok_button, NULL);

  /* sample selection tree columns */
  gxk_tree_view_add_text_column (self->tview, BST_FILE_STORE_COL_WAVE_NAME, "S",
				 0.0, _("Name"), _("Sample or instrument name"),
				 NULL, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (self->tview, BST_FILE_STORE_COL_SIZE, "S",
				 1.0, _("Size"), _("File size in bytes"),
				 NULL, self, G_CONNECT_SWAPPED);
  gchar *padstring = g_strdup (_("Format")), *tip = g_strdup (_("Detected file format"));
  guint l = strlen (padstring), n = 14;
  if (l < n)
    {
      GString *gstring = g_string_new (padstring);
      g_free (padstring);
      while (l++ < n)
        g_string_append (gstring, " ");
      padstring = g_string_free (gstring, FALSE);
    }
  gxk_tree_view_add_text_column (self->tview, BST_FILE_STORE_COL_LOADER, "OF",
				 0.0, padstring, tip,
				 NULL, self, G_CONNECT_SWAPPED);
  g_free (padstring);
  g_free (tip);
  gxk_tree_view_add_text_column (self->tview, BST_FILE_STORE_COL_TIME_STR, "S",
				 0.0, _("Time"), _("File modification time"),
				 NULL, self, G_CONNECT_SWAPPED);
  if (BST_DVL_HINTS)
    gxk_tree_view_add_toggle_column (self->tview, BST_FILE_STORE_COL_LOADABLE, "",
				     0.0, "L", "Indication of whether a file is expected to be loadable",
				     NULL, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (self->tview, BST_FILE_STORE_COL_FILE, "S",
				 0.0, _("Filename"), NULL,
				 NULL, self, G_CONNECT_SWAPPED);
  
  /* pack separator and buttons */
  gtk_box_pack_end (GTK_BOX (main_box), bbox, FALSE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (main_box),
		    g_object_new (GTK_TYPE_HSEPARATOR,
				  "visible", TRUE,
				  NULL),
		    FALSE, TRUE, 0);
  
  /* setup save options */
  self->osave = g_object_new (GTK_TYPE_FRAME,
			      "label", _("Contents"),
			      "parent", self->fs->action_area,
			      NULL);
  vbox = g_object_new (GTK_TYPE_VBOX,
		       "visible", TRUE,
		       "parent", self->osave,
		       NULL);
  self->radio1 = g_object_new (GTK_TYPE_RADIO_BUTTON,
			       "label", "radio-1",
			       "visible", TRUE,
			       "parent", vbox,
			       "can_focus", FALSE,
			       NULL);
  gtk_misc_set_alignment (GTK_MISC (GTK_BIN (self->radio1)->child), 0, .5);
  self->radio2 = g_object_new (GTK_TYPE_RADIO_BUTTON,
                               "label", "radio-2",
                               "visible", TRUE,
                               "parent", vbox,
                               "group", self->radio1,
                               "can_focus", FALSE,
                               NULL);
  gtk_misc_set_alignment (GTK_MISC (GTK_BIN (self->radio2)->child), 0, .5);
  
  /* setup actions */
  g_object_connect (self->fs->ok_button, "swapped_signal::clicked", bst_file_dialog_activate, self, NULL);
  g_object_connect (self->fs->cancel_button, "swapped_signal::clicked", gxk_toplevel_delete, self, NULL);
  
  /* fixup focus and default widgets */
  gxk_dialog_set_default (GXK_DIALOG (self), self->fs->ok_button);
  gxk_dialog_set_focus (GXK_DIALOG (self), self->fs->selection_entry);
  
  /* setup remaining bits */
  bst_file_dialog_set_mode (self, NULL, 0, _("File Selection"), 0);
  gtk_window_set_type_hint (GTK_WINDOW (self), GDK_WINDOW_TYPE_HINT_DIALOG);
}

static void
bst_file_dialog_finalize (GObject *object)
{
  BstFileDialog *self = BST_FILE_DIALOG (object);
  
  bst_file_dialog_set_mode (self, NULL, 0, NULL, 0);
  g_free (self->search_path);
  self->search_filter = NULL;
  bst_file_store_destroy (self->file_store);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (bst_file_dialog_parent_class)->finalize (object);
}

static BstFileDialog*
bst_file_dialog_global_project (void)
{
  static BstFileDialog *singleton = NULL;
  if (!singleton)
    singleton = g_object_new (BST_TYPE_FILE_DIALOG, NULL);
  return singleton;
}

static BstFileDialog*
bst_file_dialog_global_wave (void)
{
  static BstFileDialog *singleton = NULL;
  if (!singleton)
    singleton = g_object_new (BST_TYPE_FILE_DIALOG, NULL);
  return singleton;
}

static BstFileDialog*
bst_file_dialog_global_effect (void)
{
  static BstFileDialog *singleton = NULL;
  if (!singleton)
    {
      const gchar *dir = bse_server_get_custom_effect_dir (BSE_SERVER);
      singleton = g_object_new (BST_TYPE_FILE_DIALOG, NULL);
      if (dir)
        {
          sfi_make_dirpath (dir);
          gtk_file_selection_complete (singleton->fs, dir);
        }
    }
  return singleton;
}

static BstFileDialog*
bst_file_dialog_global_instrument (void)
{
  static BstFileDialog *singleton = NULL;
  if (!singleton)
    {
      const gchar *dir = bse_server_get_custom_instrument_dir (BSE_SERVER);
      singleton = g_object_new (BST_TYPE_FILE_DIALOG, NULL);
      if (dir)
        {
          sfi_make_dirpath (dir);
          gtk_file_selection_complete (singleton->fs, dir);
        }
    }
  return singleton;
}

static void
parent_window_destroyed (BstFileDialog *self)
{
  gtk_widget_hide (GTK_WIDGET (self));
  bst_file_dialog_set_mode (self, NULL, 0, NULL, 0);
  gxk_toplevel_delete (GTK_WIDGET (self));
}

void
bst_file_dialog_set_mode (BstFileDialog    *self,
			  gpointer          parent_widget,
			  BstFileDialogMode mode,
			  const gchar      *fs_title,
			  SfiProxy          proxy)
{
  GtkWindow *window = GTK_WINDOW (self);
  
  g_return_if_fail (BST_IS_FILE_DIALOG (self));

  gtk_widget_hide (GTK_WIDGET (self));
  gtk_widget_hide (self->osave);
  self->mode = mode;
  g_free (self->selected);
  self->selected = NULL;

  /* reset proxy handling */
  bst_window_sync_title_to_proxy (self, proxy, fs_title);
  self->proxy = proxy;
  self->super = 0;
  
  /* cleanup connections to old parent_window */
  if (self->parent_window)
    g_signal_handlers_disconnect_by_func (self->parent_window, parent_window_destroyed, self);
  if (window->group)
    gtk_window_group_remove_window (window->group, window);
  gtk_window_set_transient_for (window, NULL);
  
  self->parent_window = parent_widget ? (GtkWindow*) gtk_widget_get_ancestor (parent_widget, GTK_TYPE_WINDOW) : NULL;

  /* setup connections to new parent_window */
  if (self->parent_window)
    {
      gtk_window_set_transient_for (window, self->parent_window);
      if (self->parent_window->group)
	gtk_window_group_add_window (self->parent_window->group, window);
      g_signal_connect_object (self->parent_window, "destroy",
			       G_CALLBACK (parent_window_destroyed),
			       self, G_CONNECT_SWAPPED);
    }

  /* allow activation */
  self->ignore_activate = FALSE;

  /* handle tree visibility */
  switch (mode & BST_FILE_DIALOG_MODE_MASK)
    {
    case BST_FILE_DIALOG_LOAD_WAVE:
      g_free (self->search_path);
      self->search_path = g_strdup (bse_server_get_sample_path (BSE_SERVER));
      self->search_filter = NULL;
      gtk_widget_show (self->spage);
      gxk_notebook_set_current_page_widget (GTK_NOTEBOOK (self->notebook), self->fpage);
      g_object_set (self->notebook, "show_border", TRUE, "show_tabs", TRUE, NULL);
      break;
    case BST_FILE_DIALOG_LOAD_WAVE_LIB:
      g_free (self->search_path);
      self->search_path = g_strdup (bse_server_get_sample_path (BSE_SERVER));
      self->search_filter = NULL;
      gtk_widget_show (self->spage);
      gxk_notebook_set_current_page_widget (GTK_NOTEBOOK (self->notebook), self->spage);
      g_object_set (self->notebook, "show_border", TRUE, "show_tabs", TRUE, NULL);
      break;
    case BST_FILE_DIALOG_MERGE_EFFECT:
      g_free (self->search_path);
      self->search_path = g_strdup (bse_server_get_effect_path (BSE_SERVER));
      self->search_filter = "*";
      gtk_widget_show (self->spage);
      gxk_notebook_set_current_page_widget (GTK_NOTEBOOK (self->notebook), self->spage);
      g_object_set (self->notebook, "show_border", TRUE, "show_tabs", TRUE, NULL);
      break;
    case BST_FILE_DIALOG_MERGE_INSTRUMENT:
      g_free (self->search_path);
      self->search_path = g_strdup (bse_server_get_instrument_path (BSE_SERVER));
      self->search_filter = "*";
      gtk_widget_show (self->spage);
      gxk_notebook_set_current_page_widget (GTK_NOTEBOOK (self->notebook), self->spage);
      g_object_set (self->notebook, "show_border", TRUE, "show_tabs", TRUE, NULL);
      break;
    default:
      g_free (self->search_path);
      self->search_path = NULL;
      self->search_filter = NULL;
      if (self->using_file_store)
        {
          self->using_file_store = FALSE;
          bst_file_store_forget_list (self->file_store);
        }
      gtk_widget_hide (self->spage);
      g_object_set (self->notebook, "show_border", FALSE, "show_tabs", FALSE, NULL);
      break;
    }
}

GtkWidget*
bst_file_dialog_popup_open_project (gpointer parent_widget)
{
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_OPEN_PROJECT,
			    _("Open Project"), 0);
  gxk_widget_showraise (widget);

  return widget;
}

GtkWidget*
bst_file_dialog_popup_select_file (gpointer parent_widget)
{
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_SELECT_FILE,
			    _("Select File"), 0);
  gxk_widget_showraise (widget);

  return widget;
}

GtkWidget*
bst_file_dialog_popup_select_dir (gpointer parent_widget)
{
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_SELECT_DIR | BST_FILE_DIALOG_ALLOW_DIRS,
			    _("Select Directory"), 0);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_open_project (BstFileDialog *self,
			      const gchar   *file_name)
{
  SfiProxy project = bse_server_use_new_project (BSE_SERVER, file_name);
  BseErrorType error = bst_project_restore_from_file (project, file_name, TRUE, TRUE);

  if (error)
    bst_status_eprintf (error, _("Opening project `%s'"), file_name);
  else
    {
      bse_project_get_wave_repo (project);
      BstApp *app = bst_app_new (project);
      gxk_status_window_push (app);
      bst_status_eprintf (error, _("Opening project `%s'"), file_name);
      gxk_status_window_pop ();
      gxk_idle_show_widget (GTK_WIDGET (app));
    }
  bse_item_unuse (project);

  return TRUE;
}

GtkWidget*
bst_file_dialog_popup_merge_project (gpointer   parent_widget,
				     SfiProxy   project)
{
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_MERGE_PROJECT,
			    _("Merge: %s"), project);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_merge_project (BstFileDialog *self,
			       const gchar   *file_name)
{
  SfiProxy project = bse_item_use (self->proxy);
  BseErrorType error = bst_project_restore_from_file (project, file_name, FALSE, FALSE);

  bst_status_eprintf (error, _("Merging project `%s'"), file_name);

  bse_item_unuse (project);

  return TRUE;
}

GtkWidget*
bst_file_dialog_popup_import_midi (gpointer   parent_widget,
                                   SfiProxy   project)
{
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_IMPORT_MIDI,
			    _("Import MIDI: %s"), project);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_import_midi (BstFileDialog *self,
                             const gchar   *file_name)
{
  BseErrorType error = bst_project_import_midi_file (self->proxy, file_name);
  bst_status_eprintf (error, _("Importing MIDI file `%s'"), file_name);
  return TRUE;
}

static gboolean
store_bse_file (SfiProxy       project,
                SfiProxy       super,
                const gchar   *file_name,
                const gchar   *saving_message_format,
                gboolean       self_contained,
                gboolean       want_overwrite)
{
  BseErrorType error = bse_project_store_bse (project, super, file_name, self_contained);
  gchar *title = g_strdup_printf (saving_message_format, bse_item_get_name (super ? super : project));
  gboolean handled = TRUE;
  gchar *msg = NULL;
  /* handle file exists cases */
  if (error == BSE_ERROR_FILE_EXISTS)
    {
      if (!want_overwrite)
        {
          gchar *text = g_strdup_printf (_("Failed to save\n`%s'\nto\n`%s':\n%s"), bse_item_get_name (project), file_name, bse_error_blurb (error));
          GtkWidget *choice = bst_choice_dialog_createv (BST_CHOICE_TITLE (title),
                                                         BST_CHOICE_TEXT (text),
                                                         BST_CHOICE_D (1, BST_STOCK_OVERWRITE, NONE),
                                                         BST_CHOICE (0, BST_STOCK_CANCEL, NONE),
                                                         BST_CHOICE_END);
          g_free (text);
          want_overwrite = bst_choice_modal (choice, 0, 0) == 1;
          bst_choice_destroy (choice);
        }
      if (want_overwrite)
        {
          /* save to temporary file */
          gchar *temp_file = NULL;
          while (error == BSE_ERROR_FILE_EXISTS)
            {
              g_free (temp_file);
              temp_file = g_strdup_printf ("%s.tmp%06xyXXXXXX", file_name, rand() & 0xfffffd);
              mktemp (temp_file); /* this is save, due to use of: O_CREAT | O_EXCL */
              error = bse_project_store_bse (project, super, temp_file, self_contained);
            }
          /* replace file by temporary file */
          if (error != BSE_ERROR_NONE)
            {
              unlink (temp_file); /* error != BSE_ERROR_FILE_EXISTS */
              msg = g_strdup_printf (_("Failed to save to file\n`%s'\ndue to:\n%s"), file_name, bse_error_blurb (error));
            }
          else if (rename (temp_file, file_name) < 0)
            {
              unlink (temp_file);
              msg = g_strdup_printf (_("Failed to replace file\n`%s'\ndue to:\n%s"), file_name, g_strerror (errno));
            }
          else /* success */
            ;
        }
      else
        handled = FALSE;        /* exists && !overwrite */
    }
  else if (error != BSE_ERROR_NONE)
    msg = g_strdup_printf (_("Failed to save to file\n`%s'\ndue to:\n%s"), file_name, bse_error_blurb (error));
  /* report errors */
  if (msg)
    {
      GtkWidget *choice = bst_choice_dialog_createv (BST_CHOICE_TITLE (title),
                                                     BST_CHOICE_TEXT (msg),
                                                     BST_CHOICE_D (0, BST_STOCK_CLOSE, NONE),
                                                     BST_CHOICE_END);
      g_free (msg);
      bst_choice_modal (choice, 0, 0);
      bst_choice_destroy (choice);
      handled = FALSE;
    }
  else if (handled) /* no error */
    bst_status_eprintf (BSE_ERROR_NONE, "%s", title);
  g_free (title);
  return handled;
}

static gboolean
bst_file_dialog_save_project (SfiProxy     proxy,
                              gboolean     self_contained,
			      const gchar *file_name,
                              gboolean     apply_project_name,
                              gboolean     want_overwrite)
{
  SfiProxy project = bse_item_use (proxy);
  gboolean handled = store_bse_file (project, 0, file_name, _("Saving project `%s'"), self_contained, want_overwrite);
  if (apply_project_name)
    {
      bse_proxy_set_data_full (project, "beast-project-file-name", g_strdup (file_name), g_free);
      bse_proxy_set_data (project, "beast-project-store-references", (void*) !self_contained);
      gchar *bname = g_path_get_basename (file_name);
      bse_project_change_name (project, bname);
      g_free (bname);
    }
  if (handled)
    bse_project_clean_dirty (project);
  bse_item_unuse (project);

  return handled;
}

GtkWidget*
bst_file_dialog_popup_save_project (gpointer   parent_widget,
				    SfiProxy   project,
                                    gboolean   query_project_name,
                                    gboolean   apply_project_name)
{
  /* handle non-popup case */
  const gchar *filename = bse_proxy_get_data (project, "beast-project-file-name");
  gboolean store_references = (int) bse_proxy_get_data (project, "beast-project-store-references");
  if (filename && !query_project_name)
    {
      gboolean handled = bst_file_dialog_save_project (project, !store_references, filename, FALSE, TRUE);
      if (handled)
        return NULL;
    }
  /* the usual Save As scenario */
  BstFileDialog *self = bst_file_dialog_global_project ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_SAVE_PROJECT,
			    _("Save: %s"), project);
  self->apply_project_name = apply_project_name != FALSE;
  gtk_file_selection_set_filename (self->fs, filename ? filename : "");
  /* setup radio buttons */
  g_object_set (GTK_BIN (self->radio1)->child, "label", _("Fully include wave files"), NULL);
  g_object_set (GTK_BIN (self->radio2)->child, "label", _("Store references to wave files"), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->radio2), store_references);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->radio1), !store_references);
  gtk_widget_show (self->osave);
  /* show dialog */
  gxk_widget_showraise (widget);

  return widget;
}

GtkWidget*
bst_file_dialog_popup_merge_effect (gpointer   parent_widget,
                                    SfiProxy   project)
{
  BstFileDialog *self = bst_file_dialog_global_effect ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
                            BST_FILE_DIALOG_MERGE_EFFECT,
                            _("Load Effect"), project);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_merge_effect (BstFileDialog *self,
                              const gchar   *file_name)
{
  SfiProxy project = bse_item_use (self->proxy);
  BseErrorType error = bst_project_restore_from_file (project, file_name, FALSE, FALSE);

  bst_status_eprintf (error, _("Merging effect `%s'"), file_name);

  bse_item_unuse (project);

  return TRUE;
}

GtkWidget*
bst_file_dialog_popup_save_effect (gpointer parent_widget,
                                   SfiProxy project,
                                   SfiProxy super)
{
  BstFileDialog *self = bst_file_dialog_global_effect ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_SAVE_EFFECT,
			    _("Save Effect"), project);
  self->super = super;
  /* show dialog */
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_save_effect (BstFileDialog *self,
                             const gchar   *file_name)
{
  SfiProxy project = bse_item_use (self->proxy);
  gboolean self_contained = TRUE;
  gboolean handled = store_bse_file (project, self->super, file_name, _("Saving effect `%s'"), self_contained, FALSE);
  bse_item_unuse (project);

  return handled;
}

GtkWidget*
bst_file_dialog_popup_merge_instrument (gpointer   parent_widget,
                                        SfiProxy   project)
{
  BstFileDialog *self = bst_file_dialog_global_instrument ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
                            BST_FILE_DIALOG_MERGE_INSTRUMENT,
                            _("Load Instrument"), project);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_merge_instrument (BstFileDialog *self,
                                  const gchar   *file_name)
{
  SfiProxy project = bse_item_use (self->proxy);
  BseErrorType error = bst_project_restore_from_file (project, file_name, FALSE, FALSE);

  bst_status_eprintf (error, _("Merging instrument `%s'"), file_name);

  bse_item_unuse (project);

  return TRUE;
}

GtkWidget*
bst_file_dialog_popup_save_instrument (gpointer parent_widget,
                                       SfiProxy project,
                                       SfiProxy super)
{
  BstFileDialog *self = bst_file_dialog_global_instrument ();
  GtkWidget *widget = GTK_WIDGET (self);
  
  bst_file_dialog_set_mode (self, parent_widget,
			    BST_FILE_DIALOG_SAVE_INSTRUMENT,
			    _("Save Instrument"), project);
  self->super = super;
  /* show dialog */
  gxk_widget_showraise (widget);
  
  return widget;
}

static gboolean
bst_file_dialog_save_instrument (BstFileDialog *self,
                                 const gchar   *file_name)
{
  SfiProxy project = bse_item_use (self->proxy);
  gboolean self_contained = TRUE;
  gboolean handled = store_bse_file (project, self->super, file_name, _("Saving instrument `%s'"), self_contained, FALSE);
  bse_item_unuse (project);

  return handled;
}

GtkWidget*
bst_file_dialog_popup_load_wave (gpointer parent_widget,
				 SfiProxy wave_repo,
				 gboolean show_lib)
{
  BstFileDialog *self = bst_file_dialog_global_wave ();
  GtkWidget *widget = GTK_WIDGET (self);

  bst_file_dialog_set_mode (self, parent_widget,
			    show_lib ? BST_FILE_DIALOG_LOAD_WAVE_LIB : BST_FILE_DIALOG_LOAD_WAVE,
			    _("Load Wave"), wave_repo);
  gxk_widget_showraise (widget);

  return widget;
}

static gboolean
bst_file_dialog_load_wave (BstFileDialog *self,
			   const gchar   *file_name)
{
  BseErrorType error;

  gxk_status_printf (0, NULL, _("Loading wave `%s'"), file_name);
  error = bse_wave_repo_load_file (self->proxy, file_name);
  bst_status_eprintf (error, _("Loading wave `%s'"), file_name);
  if (error)
    sfi_error (_("Failed to load wave file \"%s\": %s"), file_name, bse_error_blurb (error));
  
  return TRUE;
}

GtkWidget*
bst_file_dialog_create (void)
{
  BstFileDialog *self = g_object_new (BST_TYPE_FILE_DIALOG, NULL);
  bst_file_dialog_set_mode (self, NULL,
			    BST_FILE_DIALOG_SELECT_FILE,
			    "File Selector", 0);
  return GTK_WIDGET (self);
}

void
bst_file_dialog_setup (GtkWidget        *widget,
                       gpointer          parent_widget,
                       const gchar      *title,
                       const gchar      *search_path)
{
  BstFileDialog *self = BST_FILE_DIALOG (widget);
  gchar *path;
  bst_file_dialog_set_mode (self, parent_widget,
                            BST_FILE_DIALOG_SELECT_FILE,
                            title, 0);
  g_free (self->search_path);
  self->search_path = g_strdup (search_path);
  self->search_filter = "*";
  path = g_strconcat (self->search_path, G_DIR_SEPARATOR_S, self->search_filter, NULL);
  gtk_file_selection_complete (self->fs, path);
  g_free (path);
  tree_viewable_changed (self);
}

typedef struct {
  BstFileDialogHandler handler;
  gpointer             data;
  GDestroyNotify       destroy;
} BstFileDialogData;

static void
bst_file_dialog_handler (BstFileDialog     *self,
                         BstFileDialogData *data)
{
  if (data->handler && self->selected)
    data->handler (GTK_WIDGET (self), self->selected, data->data);
  if (data->destroy)
    data->destroy (data->data);
  g_object_disconnect (self, "any_signal", bst_file_dialog_handler, data, NULL);
  g_free (data);
}

void
bst_file_dialog_set_handler (BstFileDialog    *self,
                             BstFileDialogHandler handler,
                             gpointer          handler_data,
                             GDestroyNotify    destroy)
{
  BstFileDialogData *data = g_new0 (BstFileDialogData, 1);

  g_return_if_fail (GTK_WIDGET_VISIBLE (self));
  
  data->handler = handler;
  data->data = handler_data;
  data->destroy = destroy;
  g_object_connect (self, "signal_after::hide", bst_file_dialog_handler, data, NULL);
}

static void
bst_file_dialog_activate (BstFileDialog *self)
{
  GtkWindow *swin = self->parent_window;
  gboolean popdown = TRUE;
  gchar *file_name;

  if (self->ignore_activate)
    return;

  if (self->tview && gxk_widget_viewable (GTK_WIDGET (self->tview)))
    {
      GtkTreeIter iter;
      GtkTreeModel *model;
      if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (self->tview), &model, &iter))
	{
	  GValue value = { 0, };
	  gtk_tree_model_get_value (model, &iter, BST_FILE_STORE_COL_FILE, &value);
	  file_name = g_value_dup_string (&value);
	  g_value_unset (&value);
	}
      else
	return;
    }
  else
    file_name = g_strdup (gtk_file_selection_get_filename (self->fs));

  if (!(self->mode & BST_FILE_DIALOG_ALLOW_DIRS) &&
      g_file_test (file_name, G_FILE_TEST_IS_DIR))
    {
      gchar *tmp = g_strconcat (file_name, G_DIR_SEPARATOR_S, NULL); /* don't complete on "." but "./" */
      gxk_notebook_set_current_page_widget (GTK_NOTEBOOK (self->notebook), self->fpage);
      gtk_file_selection_complete (self->fs, tmp);
      g_free (tmp);
      g_free (file_name);
      return;
    }

  if (swin)
    gxk_status_window_push (swin);
  switch (self->mode & BST_FILE_DIALOG_MODE_MASK)
    {
    case BST_FILE_DIALOG_OPEN_PROJECT:
      popdown = bst_file_dialog_open_project (self, file_name);
      break;
    case BST_FILE_DIALOG_MERGE_PROJECT:
      popdown = bst_file_dialog_merge_project (self, file_name);
      break;
    case BST_FILE_DIALOG_IMPORT_MIDI:
      popdown = bst_file_dialog_import_midi (self, file_name);
      break;
    case BST_FILE_DIALOG_MERGE_EFFECT:
      popdown = bst_file_dialog_merge_effect (self, file_name);
      break;
    case BST_FILE_DIALOG_MERGE_INSTRUMENT:
      popdown = bst_file_dialog_merge_instrument (self, file_name);
      break;
    case BST_FILE_DIALOG_SAVE_PROJECT:
      popdown = bst_file_dialog_save_project (self->proxy, GTK_TOGGLE_BUTTON (self->radio1)->active, file_name, self->apply_project_name, FALSE);
      break;
    case BST_FILE_DIALOG_SAVE_EFFECT:
      popdown = bst_file_dialog_save_effect (self, file_name);
      break;
    case BST_FILE_DIALOG_SAVE_INSTRUMENT:
      popdown = bst_file_dialog_save_instrument (self, file_name);
      break;
    case BST_FILE_DIALOG_SELECT_FILE:
    case BST_FILE_DIALOG_SELECT_DIR:
      popdown = TRUE;   /* handled via BstFileDialogHandler and ->selected */
      break;
    case BST_FILE_DIALOG_LOAD_WAVE:
    case BST_FILE_DIALOG_LOAD_WAVE_LIB:
      popdown = bst_file_dialog_load_wave (self, file_name);
      break;
    }
  if (swin)
    gxk_status_window_pop ();
  if (popdown)
    {
      /* ignore_activate guards against multiple clicks from long loads */
      self->ignore_activate = TRUE;
      g_free (self->selected);
      self->selected = file_name;
      gxk_toplevel_delete (GTK_WIDGET (self));
    }
  else
    g_free (file_name);
}

/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2002 Tim Janik and Red Hat, Inc.
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
#include "bstradiotools.h"

#include <ctype.h>


struct _BstRadioToolEntry
{
  guint             tool_id;
  BstRadioToolFlags flags;
  gchar            *name;
  gchar            *tip;
  gchar            *blurb;
  BseIcon          *icon;
  gchar            *stock_icon;
};


/* --- prototypes --- */
static void	  bst_radio_tools_class_init		(BstRadioToolsClass	*klass);
static void	  bst_radio_tools_init			(BstRadioTools		*rtools,
							 BstRadioToolsClass     *class);
static void	  bst_radio_tools_real_dispose		(GObject		*object);
static void	  bst_radio_tools_finalize		(GObject		*object);
static void	  bst_radio_tools_do_set_tool		(BstRadioTools		*rtools,
							 guint         		 tool_id);


/* --- static variables --- */
static gpointer parent_class = NULL;
static guint    signal_set_tool = 0;


/* --- functions --- */
GType
bst_radio_tools_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (BstRadioToolsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bst_radio_tools_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (BstRadioTools),
	0,      /* n_preallocs */
	(GInstanceInitFunc) bst_radio_tools_init,
      };
      type = g_type_register_static (G_TYPE_OBJECT,
				     "BstRadioTools",
				     &type_info, 0);
    }
  return type;
}

static void
bst_radio_tools_class_init (BstRadioToolsClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bst_radio_tools_real_dispose;
  gobject_class->finalize = bst_radio_tools_finalize;

  class->set_tool = bst_radio_tools_do_set_tool;

  signal_set_tool = g_signal_new ("set-tool",
				  G_OBJECT_CLASS_TYPE (class),
				  G_SIGNAL_RUN_LAST | GTK_RUN_NO_RECURSE,
				  G_STRUCT_OFFSET (BstRadioToolsClass, set_tool),
				  NULL, NULL,
				  bst_marshal_NONE__UINT,
				  G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
bst_radio_tools_init (BstRadioTools      *rtools,
		      BstRadioToolsClass *class)
{
  rtools->block_tool_id = 0;
  rtools->tool_id = 0;
  rtools->n_tools = 0;
  rtools->tools = NULL;
  rtools->widgets = NULL;
}

static void
bst_radio_tools_real_dispose (GObject *object)
{
  BstRadioTools *self = BST_RADIO_TOOLS (object);

  bst_radio_tools_clear_tools (self);

  while (self->widgets)
    gtk_widget_destroy (self->widgets->data);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bst_radio_tools_finalize (GObject *object)
{
  BstRadioTools *self = BST_RADIO_TOOLS (object);

  bst_radio_tools_clear_tools (self);

  while (self->widgets)
    gtk_widget_destroy (self->widgets->data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BstRadioTools*
bst_radio_tools_new (void)
{
  BstRadioTools *rtools = g_object_new (BST_TYPE_RADIO_TOOLS, NULL);
  
  return rtools;
}

static void
bst_radio_tools_do_set_tool (BstRadioTools *rtools,
			     guint          tool_id)
{
  GSList *slist, *next;

  rtools->block_tool_id++;
  for (slist = rtools->widgets; slist; slist = next)
    {
      GtkWidget *widget = slist->data;

      next = slist->next;
      if (GTK_IS_TOGGLE_BUTTON (widget))
	{
	  tool_id = g_object_get_long (widget, "user_data");
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), tool_id == rtools->tool_id);
	}
      else if (gxk_toolbar_choice_is_item (widget))
	{
	  tool_id = g_object_get_long (widget, "user_data");
	  if (tool_id == rtools->tool_id &&
	      !gxk_toolbar_choice_is_selected (widget))
	    gxk_toolbar_choice_select (widget);
	}
    }
  rtools->block_tool_id--;
}

void
bst_radio_tools_set_tool (BstRadioTools *rtools,
			  guint          tool_id)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  if (!rtools->block_tool_id)
    {
      /* emit the signal unconditionally if we don't have tools yet */
      if (!rtools->n_tools || rtools->tool_id != tool_id)
	{
	  rtools->tool_id = tool_id;
	  g_signal_emit (rtools, signal_set_tool, 0, tool_id);
	}
    }
}

void
bst_radio_tools_add_category (BstRadioTools    *rtools,
			      guint             tool_id,
			      BseCategory      *category,
			      BstRadioToolFlags flags)
{
  gchar *tip;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (category != NULL);
  g_return_if_fail (flags != 0);

#if 0
  guint i, next_uc = 0;

  /* strip first namespace prefix from type name */
  name = g_type_name (category->type);
  for (i = 0; name[i] != 0; i++)
    if (i && toupper (name[i]) == name[i])
      {
	next_uc = i;
	break;
      }
  if (toupper (name[0]) == name[0] && next_uc > 0)
    name += next_uc;
#endif
  
  tip = g_strconcat (category->category + category->lindex + 1,
		     " [", category->type, "]",
		     NULL);
  bst_radio_tools_add_tool (rtools,
			    tool_id,
			    category->category + category->lindex + 1,
			    tip,
			    bse_type_blurb (category->type),
			    category->icon,
			    flags);
  g_free (tip);
}

static void
bst_radio_tools_add_tool_any (BstRadioTools    *rtools,
			      guint             tool_id,
			      const gchar      *tool_name,
			      const gchar      *tool_tip,
			      const gchar      *tool_blurb,
			      BseIcon          *tool_icon,
			      const gchar      *stock_icon,
			      BstRadioToolFlags flags)
{
  guint i;

  if (!tool_tip)
    tool_tip = tool_name;
  if (!tool_blurb)
    tool_blurb = tool_tip;

  i = rtools->n_tools++;
  rtools->tools = g_renew (BstRadioToolEntry, rtools->tools, rtools->n_tools);
  rtools->tools[i].tool_id = tool_id;
  rtools->tools[i].name = g_strdup (tool_name);
  rtools->tools[i].tip = g_strdup (tool_tip);
  rtools->tools[i].blurb = g_strdup (tool_blurb);
  rtools->tools[i].icon = tool_icon ? bse_icon_copy_shallow (tool_icon) : NULL;
  rtools->tools[i].stock_icon = g_strdup (stock_icon);
  rtools->tools[i].flags = flags;
}

void
bst_radio_tools_add_tool (BstRadioTools    *rtools,
			  guint             tool_id,
			  const gchar      *tool_name,
			  const gchar      *tool_tip,
			  const gchar      *tool_blurb,
			  BseIcon          *tool_icon,
			  BstRadioToolFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);

  bst_radio_tools_add_tool_any (rtools, tool_id, tool_name, tool_tip, tool_blurb, tool_icon, NULL, flags);
}

void
bst_radio_tools_add_stock_tool (BstRadioTools    *rtools,
				guint             tool_id,
				const gchar      *tool_name,
				const gchar      *tool_tip,
				const gchar      *tool_blurb,
				const gchar	 *stock_icon,
				BstRadioToolFlags flags)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);

  bst_radio_tools_add_tool_any (rtools, tool_id, tool_name, tool_tip, tool_blurb, NULL, stock_icon, flags);
}

void
bst_radio_tools_clear_tools (BstRadioTools *rtools)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  for (i = 0; i < rtools->n_tools; i++)
    {
      g_free (rtools->tools[i].name);
      g_free (rtools->tools[i].tip);
      g_free (rtools->tools[i].blurb);
      if (rtools->tools[i].icon)
	bse_icon_free (rtools->tools[i].icon);
      g_free (rtools->tools[i].stock_icon);
    }
  rtools->n_tools = 0;
  g_free (rtools->tools);
  rtools->tools = NULL;
}

void
bst_radio_tools_destroy (BstRadioTools *rtools)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  g_object_run_dispose (G_OBJECT (rtools));
}

static void
rtools_widget_destroyed (BstRadioTools *rtools,
			 GtkWidget     *widget)
{
  rtools->widgets = g_slist_remove (rtools->widgets, widget);
}

static void
rtools_toggle_toggled (BstRadioTools   *rtools,
		       GtkToggleButton *toggle)
{
  guint tool_id;

  if (!rtools->block_tool_id)
    {
      GdkEvent *event = gtk_get_current_event ();
      tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (toggle)));
      /* ignore untoggeling through the GUI (button release on depressed toggle) */
      if (toggle->active ||
	  (gtk_get_event_widget (event) == GTK_WIDGET (toggle) &&
	   event->type == GDK_BUTTON_RELEASE))
	bst_radio_tools_set_tool (rtools, tool_id);
      else
	bst_radio_tools_set_tool (rtools, 0);
      /* enforce depressed state in case tool_id didn't change */
      if (rtools->tool_id == tool_id && !toggle->active)
	{
	  rtools->block_tool_id++;
	  gtk_toggle_button_set_active (toggle, TRUE);
	  rtools->block_tool_id--;
	}
    }
}

void
bst_radio_tools_build_toolbar (BstRadioTools *rtools,
			       GxkToolbar    *toolbar)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (GXK_IS_TOOLBAR (toolbar));

  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (rtools->tools[i].stock_icon)
	image = gxk_stock_image (rtools->tools[i].stock_icon, BST_SIZE_TOOLBAR);
      if (!image)
	image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_TOOLBAR);
      button = gxk_toolbar_append (toolbar, GXK_TOOLBAR_TOGGLE,
				   rtools->tools[i].name,
				   rtools->tools[i].tip,
				   image);
      g_object_set (button,
		    "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
		    NULL);
      g_object_connect (button,
			"swapped_signal::toggled", rtools_toggle_toggled, rtools,
			"swapped_signal::destroy", rtools_widget_destroyed, rtools,
			NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
    }

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

static void
rtools_choice_func (gpointer       data,
		    guint          tool_id)
{
  BstRadioTools *rtools = BST_RADIO_TOOLS (data);

  if (!rtools->block_tool_id)
    bst_radio_tools_set_tool (rtools, tool_id);
}

void
bst_radio_tools_build_toolbar_choice (BstRadioTools *rtools,
				      GxkToolbar    *toolbar)
{
  GtkWidget *choice_widget;
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (GXK_IS_TOOLBAR (toolbar));

  choice_widget = gxk_toolbar_append_choice (toolbar, GXK_TOOLBAR_TRUNC_BUTTON,
					     rtools_choice_func, rtools, NULL);
  rtools->block_tool_id++;
  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *item, *image = NULL;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (rtools->tools[i].stock_icon)
	image = gxk_stock_image (rtools->tools[i].stock_icon, BST_SIZE_TOOLBAR);
      if (!image)
	image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_TOOLBAR);
      item = gxk_toolbar_choice_add (choice_widget,
				     rtools->tools[i].name,
				     rtools->tools[i].tip,
				     image,
				     rtools->tools[i].tool_id);
      g_object_set_long (item, "user_data", rtools->tools[i].tool_id);
      g_object_connect (item,
			"swapped_signal::destroy", rtools_widget_destroyed, rtools,
			NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, item);
    }
  rtools->block_tool_id--;

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

void
bst_radio_tools_build_gtk_toolbar (BstRadioTools *rtools,
				   GtkToolbar    *toolbar)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (GTK_IS_TOOLBAR (toolbar));

  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_TOOLBAR);
      else if (rtools->tools[i].stock_icon)
	image = gxk_stock_image (rtools->tools[i].stock_icon, BST_SIZE_TOOLBAR);
      if (!image)
	image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_TOOLBAR);
      button = gtk_toolbar_append_element (toolbar,
					   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
					   rtools->tools[i].name,
					   rtools->tools[i].tip, NULL,
					   image,
					   NULL, NULL);
      g_object_set (button,
		    "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
		    NULL);
      g_object_connect (button,
			"swapped_signal::toggled", rtools_toggle_toggled, rtools,
			"swapped_signal::destroy", rtools_widget_destroyed, rtools,
			NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
    }

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

static void
toggle_apply_blurb (GtkToggleButton *toggle,
		    GtkWidget       *text)
{
  gpointer tool_id = gtk_object_get_data (GTK_OBJECT (toggle), "user_data");
  gpointer blurb_id = gtk_object_get_data (GTK_OBJECT (text), "user_data");

  if (tool_id == blurb_id && !toggle->active)
    {
      gxk_scroll_text_set (text, NULL);
      g_object_set_data (G_OBJECT (text), "user_data", GUINT_TO_POINTER (~0));
    }
  else if (toggle->active && tool_id != blurb_id)
    {
      gxk_scroll_text_set (text, gtk_object_get_data (GTK_OBJECT (toggle), "blurb"));
      g_object_set_data (G_OBJECT (text), "user_data", tool_id);
    }
}

GtkWidget*
bst_radio_tools_build_palette (BstRadioTools *rtools,
			       gboolean       show_descriptions,
			       GtkReliefStyle relief)
{
  GtkWidget *vbox, *table, *text = NULL;
  guint i, n = 0, column = 5;
  
  g_return_val_if_fail (BST_IS_RADIO_TOOLS (rtools), NULL);
  
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "resize_mode", GTK_RESIZE_QUEUE,
			 NULL);
  table = gtk_widget_new (GTK_TYPE_TABLE,
			  "visible", TRUE,
			  "homogeneous", TRUE,
			  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  if (show_descriptions)
    {
      text = gxk_scroll_text_create (0, NULL);
      g_object_set_data (G_OBJECT (text), "user_data", GUINT_TO_POINTER (~0));
      gtk_widget_ref (text);
      gtk_object_sink (GTK_OBJECT (text));
    }
  
  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *image = NULL;
      
      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_PALETTE))
	continue;
      
      if (rtools->tools[i].icon)
	image = bst_image_from_icon (rtools->tools[i].icon, BST_SIZE_PALETTE);
      else if (rtools->tools[i].stock_icon)
	image = gxk_stock_image (rtools->tools[i].stock_icon, BST_SIZE_PALETTE);
      if (!image)
	image = gxk_stock_image (BST_STOCK_NO_ICON, BST_SIZE_PALETTE);
      button = g_object_connect (gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
						 "visible", TRUE,
						 "draw_indicator", FALSE,
						 "relief", relief,
						 "child", image,
						 "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
						 NULL),
				 "swapped_signal::toggled", rtools_toggle_toggled, rtools,
				 "swapped_signal::destroy", rtools_widget_destroyed, rtools,
				 text ? "signal_after::toggled" : NULL, toggle_apply_blurb, text,
				 NULL);
      gtk_tooltips_set_tip (BST_TOOLTIPS, button, rtools->tools[i].tip, NULL);
      gtk_object_set_data_full (GTK_OBJECT (button), "blurb", g_strdup (rtools->tools[i].blurb), g_free);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
      gtk_table_attach (GTK_TABLE (table),
			button,
			n % column, n % column + 1,
			n / column, n / column + 1,
			GTK_SHRINK, GTK_SHRINK, // GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			0, 0);
      n++;
    }
  
  if (n && text)
    {
      GtkWidget *hbox;

      hbox = gtk_widget_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     NULL);
      gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 5);
      gtk_widget_new (GTK_TYPE_FRAME,
		      "visible", TRUE,
		      "label", "Description",
		      "label_xalign", 0.5,
		      "border_width", 5,
		      "parent", vbox,
		      "child", hbox,
		      "width_request", 1,
		      "height_request", 100,
		      NULL);
    }
  if (text)
    gtk_widget_unref (text);
  
  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
  
  return vbox;
}

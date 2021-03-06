/* BEAST - Better Audio System
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bstcanvassource.h"

#include "topconfig.h"
#include "bstparamview.h"
#include "bstgconfig.h"
#include <string.h>


/* --- defines --- */
#define	ICON_WIDTH(cs)		((gdouble) 64)
#define	ICON_HEIGHT(cs)		((gdouble) 64)
#define CHANNEL_WIDTH(cs)	((gdouble) 10)
#define CHANNEL_HEIGHT(cs)	((gdouble) ICON_HEIGHT (cs))
#define	ICON_X(cs)		((gdouble) CHANNEL_WIDTH (cs))
#define	ICON_Y(cs)		((gdouble) 0)
#define ICHANNEL_realX(cs)	((gdouble) 0)
#define OCHANNEL_realX(cs)	((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs))
#define ICHANNEL_X(cs)		(cs->swap_channels ? OCHANNEL_realX (cs) : ICHANNEL_realX (cs))
#define OCHANNEL_X(cs)		(cs->swap_channels ? ICHANNEL_realX (cs) : OCHANNEL_realX (cs))
#define CHANNEL_EAST(cs,isinp)	(cs->swap_channels ^ !isinp)
#define	BORDER_PAD		((gdouble) 1)
#define ICHANNEL_Y(cs)		((gdouble) 0)
#define OCHANNEL_Y(cs)		((gdouble) 0)
#define	TOTAL_WIDTH(cs)		((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs) + CHANNEL_WIDTH (cs))
#define	TOTAL_HEIGHT(cs)	((gdouble) ICON_HEIGHT (cs))
#define TEXT_X(cs)		((gdouble) CHANNEL_WIDTH (cs) + ICON_WIDTH (cs) / 2)    /* for anchor: center */
#define TEXT_Y(cs)		((gdouble) ICON_HEIGHT (cs))                            /* for anchor: north */
#define TEXT_HEIGHT		((gdouble) FONT_HEIGHT + 2)
#define	CHANNEL_FONT		("Sans")
#define	TEXT_FONT		("Serif")
#define	FONT_HEIGHT		((gdouble) BST_GCONFIG (snet_font_size))
#define RGBA_BLACK		(0x000000ff)
#define RGBA_INTERNAL           (0x0000ffff)

/* --- signals --- */
enum
{
  SIGNAL_UPDATE_LINKS,
  SIGNAL_LAST
};
typedef void    (*SignalUpdateLinks)            (BstCanvasSource       *source,
						 gpointer         func_data);


/* --- prototypes --- */
static gboolean bst_canvas_source_child_event	(BstCanvasSource	*csource,
						 GdkEvent               *event,
						 GnomeCanvasItem        *child);
static void     bst_canvas_source_changed       (BstCanvasSource        *csource);
static void	bst_canvas_icon_set		(GnomeCanvasItem	*item,
						 BseIcon         	*icon,
                                                 const gchar            *module_type);
static void	csource_info_update		(BstCanvasSource	*csource);
static void     bst_canvas_source_build         (BstCanvasSource        *csource);


/* --- static variables --- */
static guint                 csource_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
G_DEFINE_TYPE (BstCanvasSource, bst_canvas_source, GNOME_TYPE_CANVAS_GROUP);

static void
bst_canvas_source_init (BstCanvasSource *csource)
{
  GtkObject *object = GTK_OBJECT (csource);
  
  csource->source = 0;
  csource->params_dialog = NULL;
  csource->source_info = NULL;
  csource->icon_item = NULL;
  csource->text = NULL;
  csource->channel_items = NULL;
  csource->channel_hints = NULL;
  csource->swap_channels = BST_SNET_SWAP_IO_CHANNELS;
  csource->in_move = FALSE;
  csource->show_hints = FALSE;
  csource->move_dx = 0;
  csource->move_dy = 0;
  g_object_connect (object,
		    "signal::notify", bst_canvas_source_changed, NULL,
		    NULL);
}

static void
source_channels_changed (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  GNOME_CANVAS_NOTIFY (csource);
  bst_canvas_source_update_links (csource);
}

static gboolean
idle_move_item (gpointer data)
{
  BstCanvasSource *self = data;
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (self);

  GDK_THREADS_ENTER ();
  if (self->source && item->canvas)
    {
      SfiReal x, y;
      bse_proxy_get (self->source,
                     "pos-x", &x,
                     "pos-y", &y,
                     NULL);
      x *= BST_CANVAS_SOURCE_PIXEL_SCALE;
      y *= -BST_CANVAS_SOURCE_PIXEL_SCALE;
      gnome_canvas_item_w2i (item, &x, &y);
      g_object_freeze_notify (G_OBJECT (self));
      gnome_canvas_item_move (item, x, y);
      /* canvas notification bug workaround */
      g_object_notify (self, "x");
      g_object_notify (self, "y");
      g_object_thaw_notify (G_OBJECT (self));
    }
  self->idle_reposition = FALSE;
  g_object_unref (self);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
source_pos_changed (BstCanvasSource *self)
{
  if (!self->idle_reposition)
    {
      self->idle_reposition = TRUE;
      g_idle_add (idle_move_item, g_object_ref (self));
    }
}

static void
canvas_source_set_position (BstCanvasSource *self)
{
  gboolean idle_reposition = self->idle_reposition;
  GDK_THREADS_LEAVE ();
  idle_move_item (g_object_ref (self));
  GDK_THREADS_ENTER ();
  self->idle_reposition = idle_reposition;
}

static void
source_name_changed (BstCanvasSource *csource)
{
  const gchar *name;

  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  name = bse_item_get_name_or_type (csource->source);

  if (csource->text)
    g_object_set (csource->text, "text", name, NULL);

  if (csource->params_dialog)
    {
      gxk_dialog_set_title (GXK_DIALOG (csource->params_dialog), name);
      csource_info_update (csource);
    }

  name = g_strconcat ("Info: ", name, NULL);
  if (csource->source_info)
    gxk_dialog_set_title (GXK_DIALOG (csource->source_info), name);
}

static void
source_icon_changed (BstCanvasSource *csource)
{
  BseIcon *icon;

  /* update icon in group, revert to a stock icon if none is available
   */
  icon = bse_item_get_icon (csource->source);
  if (csource->icon_item)
    bst_canvas_icon_set (csource->icon_item, icon, bse_item_get_type (csource->source));
}

static void
bst_canvas_source_destroy (GtkObject *object)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);

  if (csource->in_move)
    {
      csource->in_move = FALSE;
      bse_item_ungroup_undo (csource->source);
    }

  while (csource->channel_hints)
    gtk_object_destroy (csource->channel_hints->data);
  while (group->item_list)
    gtk_object_destroy (group->item_list->data);

  if (csource->source)
    {
      bse_proxy_disconnect (csource->source,
			    "any_signal", gtk_object_destroy, csource,
			    "any_signal", source_channels_changed, csource,
			    "any_signal", source_name_changed, csource,
			    "any_signal", source_pos_changed, csource,
			    "any_signal", source_icon_changed, csource,
			    NULL);
      bse_item_unuse (csource->source);
      csource->source = 0;
    }

  GTK_OBJECT_CLASS (bst_canvas_source_parent_class)->destroy (object);
}

#define EPSILON 1e-6

static void
bse_object_set_parasite_coords (SfiProxy proxy,
				SfiReal  x,
				SfiReal  y)
{
  bse_source_set_pos (proxy,
                      x / BST_CANVAS_SOURCE_PIXEL_SCALE,
                      y / -BST_CANVAS_SOURCE_PIXEL_SCALE);
}

GnomeCanvasItem*
bst_canvas_source_new (GnomeCanvasGroup *group,
		       SfiProxy		 source)
{
  BstCanvasSource *csource;
  GnomeCanvasItem *item;

  g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);

  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_SOURCE,
				NULL);
  csource = BST_CANVAS_SOURCE (item);
  csource->source = bse_item_use (source);
  bse_proxy_connect (csource->source,
		     "swapped_signal::release", gtk_object_destroy, csource,
		     "swapped_signal::io_changed", source_channels_changed, csource,
		     "swapped_signal::property-notify::uname", source_name_changed, csource,
		     "swapped_signal::property-notify::pos-x", source_pos_changed, csource,
		     "swapped_signal::property-notify::pos-y", source_pos_changed, csource,
		     "swapped_signal::icon-changed", source_icon_changed, csource,
		     NULL);

  canvas_source_set_position (csource);
  bst_canvas_source_build (csource);
  
  GNOME_CANVAS_NOTIFY (item);
  
  return item;
}

void
bst_canvas_source_update_links (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (csource->source)
    gtk_signal_emit (GTK_OBJECT (csource), csource_signals[SIGNAL_UPDATE_LINKS]);
}

static inline void
canvas_source_create_params (BstCanvasSource *csource)
{
  if (!csource->params_dialog)
    {
      GtkWidget *param_view;

      param_view = bst_param_view_new (csource->source);
      csource->params_dialog = gxk_dialog_new (&csource->params_dialog,
                                               GTK_OBJECT (csource),
                                               GXK_DIALOG_POPUP_POS,
                                               bse_item_get_name_or_type (csource->source),
                                               param_view);
      source_name_changed (csource);
    }
}

void
bst_canvas_source_reset_params (BstCanvasSource *csource)
{
  GtkWidget *param_view;

  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  canvas_source_create_params (csource);
  param_view = gxk_dialog_get_child (GXK_DIALOG (csource->params_dialog));
  bst_param_view_apply_defaults (BST_PARAM_VIEW (param_view));
}

void
bst_canvas_source_popup_params (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  canvas_source_create_params (csource);
  gxk_widget_showraise (csource->params_dialog);
}

void
bst_canvas_source_toggle_params (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->params_dialog || !GTK_WIDGET_VISIBLE (csource->params_dialog))
    bst_canvas_source_popup_params (csource);
  else
    gtk_widget_hide (csource->params_dialog);
}

void
bst_canvas_source_set_channel_hints (BstCanvasSource *csource,
				     gboolean         on_off)
{
  GSList *slist;

  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  csource->show_hints = !!on_off;
  if (csource->show_hints)
    for (slist = csource->channel_hints; slist; slist = slist->next)
      g_object_set (slist->data, "text", g_object_get_data (slist->data, "hint_text"), NULL);
  else
    for (slist = csource->channel_hints; slist; slist = slist->next)
      g_object_set (slist->data, "text", "", NULL);
}

static void
csource_info_update (BstCanvasSource *csource)
{
  GtkWidget *text = (csource->source_info // && (force_update || GTK_WIDGET_VISIBLE (csource->source_info))
                     ? gxk_dialog_get_child (GXK_DIALOG (csource->source_info))
                     : NULL);
  if (text)
    {
      const gchar *string;
      guint i;

      /* construct information */
      gxk_scroll_text_clear (text);
      gxk_scroll_text_aprintf (text, "%s:\n", bse_item_get_name_or_type (csource->source));

      /* type & category */
      gxk_scroll_text_push_indent (text);
      gxk_scroll_text_aprintf (text, "Type: %s\n", bse_item_get_type_name (csource->source));
      BseCategorySeq *cseq = bse_categories_match_typed ("*", bse_item_get_type (csource->source));
      if (cseq->n_cats)
        gxk_scroll_text_aprintf (text, "Category: %s\n", cseq->cats[0]->category);
      gxk_scroll_text_pop_indent (text);

      /* input channels */
      if (bse_source_n_ichannels (csource->source))
	{
	  gxk_scroll_text_aprintf (text, "\nInput Channels:\n");
	  gxk_scroll_text_push_indent (text);
	}
      for (i = 0; i < bse_source_n_ichannels (csource->source); i++)
	{
          string = bse_source_ichannel_blurb (csource->source, i);
	  gxk_scroll_text_aprintf (text, "%s[%s]%s\n",
				   bse_source_ichannel_label (csource->source, i),
				   bse_source_ichannel_ident (csource->source, i),
				   string ? ":" : "");
	  if (string)
	    {
	      gxk_scroll_text_push_indent (text);
	      gxk_scroll_text_aprintf (text, "%s\n", string);
	      gxk_scroll_text_pop_indent (text);
	    }
	}
      if (bse_source_n_ichannels (csource->source))
	gxk_scroll_text_pop_indent (text);

      /* output channels */
      if (bse_source_n_ochannels (csource->source))
	{
	  gxk_scroll_text_aprintf (text, "\nOutput Channels:\n");
	  gxk_scroll_text_push_indent (text);
	}
      for (i = 0; i < bse_source_n_ochannels (csource->source); i++)
	{
	  string = bse_source_ochannel_blurb (csource->source, i);
	  gxk_scroll_text_aprintf (text, "%s[%s]%s\n",
				   bse_source_ochannel_label (csource->source, i),
				   bse_source_ochannel_ident (csource->source, i),
				   string ? ":" : "");
          if (string)
	    {
	      gxk_scroll_text_push_indent (text);
	      gxk_scroll_text_aprintf (text, "%s\n", string);
	      gxk_scroll_text_pop_indent (text);
	    }
	}
      if (bse_source_n_ochannels (csource->source))
	gxk_scroll_text_pop_indent (text);

      /* description */
      string = bse_item_get_type_blurb (csource->source);
      if (string && string[0])
	{
	  gxk_scroll_text_aprintf (text, "\nDescription:\n");
	  gxk_scroll_text_push_indent (text);
	  gxk_scroll_text_aprintf (text, "%s\n", string);
	  gxk_scroll_text_pop_indent (text);
	}

      /* authors */
      string = bse_item_get_type_authors (csource->source);
      if (string && string[0])
        gxk_scroll_text_aprintf (text, "\nAuthors: %s\n", string);
      
      /* license */
      string = bse_item_get_type_license (csource->source);
      if (string && string[0])
        gxk_scroll_text_aprintf (text, "\nLicense: %s\n", string);
    }
}

void
bst_canvas_source_popup_info (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info)
    {
      GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, NULL);
      GtkWidget *frame = gtk_widget_new (GTK_TYPE_FRAME,
                                         "visible", TRUE,
                                         "label", _("Module Info"),
                                         "border_width", 5,
                                         NULL);
      g_object_new (GTK_TYPE_ALIGNMENT,
                    "visible", TRUE,
                    "parent", frame,
                    "border_width", 2,
                    "child", sctext,
                    NULL),
      csource->source_info = gxk_dialog_new (&csource->source_info,
					     GTK_OBJECT (csource),
					     GXK_DIALOG_POPUP_POS,
					     bse_item_get_name_or_type (csource->source),
                                             sctext);
    }
  csource_info_update (csource);
  source_name_changed (csource);
  gxk_widget_showraise (csource->source_info);
}

void
bst_canvas_source_toggle_info (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info || !GTK_WIDGET_VISIBLE (csource->source_info))
    bst_canvas_source_popup_info (csource);
  else
    gtk_widget_hide (csource->source_info);
}

BstCanvasSource*
bst_canvas_source_at (GnomeCanvas *canvas,
		      gdouble      world_x,
		      gdouble      world_y)
{
  return (BstCanvasSource*) gnome_canvas_typed_item_at (canvas, BST_TYPE_CANVAS_SOURCE, world_x, world_y);
}

gboolean
bst_canvas_source_is_jchannel (BstCanvasSource *csource,
			       guint            ichannel)
{
  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), FALSE);

  if (!csource->source)
    return FALSE;

  return bse_source_is_joint_ichannel_by_id (csource->source, ichannel);
}

gboolean
bst_canvas_source_ichannel_free (BstCanvasSource *csource,
				 guint            ichannel)
{
  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), FALSE);

  if (!csource->source)
    return FALSE;

  if (bse_source_is_joint_ichannel_by_id (csource->source, ichannel))
    return TRUE;
  else
    return bse_source_ichannel_get_osource (csource->source, ichannel, 0) == 0;
}

void
bst_canvas_source_ichannel_pos (BstCanvasSource *csource,
				guint            ochannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x = 0, y = 0;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = ICHANNEL_X (csource) + CHANNEL_WIDTH (csource) / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT (csource) / bse_source_n_ichannels (csource->source);
  y *= ochannel + 0.5;
  y += ICHANNEL_Y (csource);
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

void
bst_canvas_source_ochannel_pos (BstCanvasSource *csource,
				guint            ichannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = OCHANNEL_X (csource) + CHANNEL_WIDTH (csource) / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT (csource) / bse_source_n_ochannels (csource->source);
  y *= ichannel + 0.5;
  y += OCHANNEL_Y (csource);
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

guint
bst_canvas_source_ichannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel = ~0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= ICHANNEL_X (csource);
  y -= ICHANNEL_Y (csource);
  if (x > 0 && x < CHANNEL_WIDTH (csource) &&
      y > 0 && y < CHANNEL_HEIGHT (csource) &&
      bse_source_n_ichannels (csource->source))
    {
      y /= CHANNEL_HEIGHT (csource) / bse_source_n_ichannels (csource->source);
      channel = y;
    }

  return channel;
}

guint
bst_canvas_source_ochannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel = ~0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= OCHANNEL_X (csource);
  y -= OCHANNEL_Y (csource);
  if (x > 0 && x < CHANNEL_WIDTH (csource) &&
      y > 0 && y < CHANNEL_HEIGHT (csource) &&
      bse_source_n_ochannels (csource->source))
    {
      y /= CHANNEL_HEIGHT (csource) / bse_source_n_ochannels (csource->source);
      channel = y;
    }

  return channel;
}

static void
bst_canvas_icon_set (GnomeCanvasItem *item,
		     BseIcon         *icon,
                     const gchar     *module_type)
{
  GdkPixbuf *pixbuf;
  gboolean need_unref = FALSE;

  if (icon && icon->pixels->bytes)
    {
      icon = bse_icon_copy_shallow (icon);
      pixbuf = gdk_pixbuf_new_from_data (icon->pixels->bytes, GDK_COLORSPACE_RGB, icon->bytes_per_pixel == 4,
					 8, icon->width, icon->height,
					 icon->width * icon->bytes_per_pixel,
					 NULL, NULL);
      g_object_set_data_full (G_OBJECT (pixbuf),
			      "BseIcon",
			      icon,
			      (GtkDestroyNotify) bse_icon_free);
      need_unref = TRUE;
    }
  else if (module_type && strncmp (module_type, "BseLadspaModule_", 16) == 0)
    pixbuf = bst_pixbuf_ladspa ();
  else
    pixbuf = bst_pixbuf_no_icon ();

  g_object_set (GTK_OBJECT (item),
		"pixbuf", pixbuf,
		"x_in_pixels", FALSE,
		"y_in_pixels", FALSE,
		"anchor", GTK_ANCHOR_NORTH_WEST,
		NULL);
  if (need_unref)
    g_object_unref (pixbuf);
}

static void
channel_item_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_items = g_slist_remove (csource->channel_items, item);
}

static void
channel_name_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_hints = g_slist_remove (csource->channel_hints, item);
}

static void
bst_canvas_source_build_channels (BstCanvasSource *csource,
				  gboolean         is_input,
				  gint             color1,
				  gint		   color1_fade,
				  gint		   color2,
				  gint		   color2_fade,
                                  gboolean         build_channel_items,
                                  gboolean         build_channel_hints)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  const guint alpha = 0xa0;
  gint n_channels, color1_delta = 0, color2_delta = 0;
  gdouble x1, x2, y1, y2;
  gdouble d_y;
  gboolean east_channel = CHANNEL_EAST (csource, is_input);
  guint i;

  if (is_input)
    {
      n_channels = bse_source_n_ichannels (csource->source);
      x1 = ICHANNEL_X (csource);
      y1 = ICHANNEL_Y (csource);
    }
  else
    {
      n_channels = bse_source_n_ochannels (csource->source);
      x1 = OCHANNEL_X (csource);
      y1 = OCHANNEL_Y (csource);
    }
  x2 = x1 + CHANNEL_WIDTH (csource);
  y2 = y1 + CHANNEL_HEIGHT (csource);
  d_y = y2 - y1;
  if (n_channels)
    d_y /= n_channels;

  if (n_channels > 1)
    {
      gint cd_red, cd_blue, cd_green;

      cd_red = ((color1_fade & 0xff0000) - (color1 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color1_fade & 0x00ff00) - (color1 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color1_fade & 0x0000ff) - (color1 & 0x0000ff)) / (n_channels - 1);
      color1_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;

      cd_red = ((color2_fade & 0xff0000) - (color2 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color2_fade & 0x00ff00) - (color2 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color2_fade & 0x0000ff) - (color2 & 0x0000ff)) / (n_channels - 1);
      color2_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;
    }
  else if (n_channels == 0)
    {
      GnomeCanvasItem *item;
      if (build_channel_items)
        {
          item = g_object_connect (gnome_canvas_item_new (group,
                                                          GNOME_TYPE_CANVAS_RECT,
                                                          "fill_color_rgba", (0xc3c3c3 << 8) | alpha,
                                                          "outline_color_rgba", RGBA_BLACK,
                                                          "x1", x1,
                                                          "y1", y1,
                                                          "x2", x2,
                                                          "y2", y2,
                                                          NULL),
                                   "swapped_signal::destroy", channel_item_remove, csource,
                                   "swapped_signal::event", bst_canvas_source_child_event, csource,
                                   NULL);
          csource->channel_items = g_slist_prepend (csource->channel_items, item);
        }
    }
  
  for (i = 0; i < n_channels; i++)
    {
      GnomeCanvasItem *item;
      gboolean is_jchannel = is_input && bse_source_is_joint_ichannel_by_id (csource->source, i);
      const gchar *label = (is_input ? bse_source_ichannel_label : bse_source_ochannel_label) (csource->source, i);
      guint tmp_color = is_jchannel ? color2 : color1;

      y2 = y1 + d_y;
      if (build_channel_items)
        {
          item = gnome_canvas_item_new (group,
                                        GNOME_TYPE_CANVAS_RECT,
                                        "fill_color_rgba", (tmp_color << 8) | alpha,
                                        "outline_color_rgba", RGBA_BLACK,
                                        "x1", x1,
                                        "y1", y1,
                                        "x2", x2,
                                        "y2", y2,
                                        NULL);
          g_object_connect (item,
                            "swapped_signal::destroy", channel_item_remove, csource,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          csource->channel_items = g_slist_prepend (csource->channel_items, item);
        }

      if (build_channel_hints)
        {
          item = gnome_canvas_item_new (group,
                                        GNOME_TYPE_CANVAS_TEXT,
                                        "fill_color_rgba", (0x000000 << 8) | 0x80,
                                        "anchor", east_channel ? GTK_ANCHOR_WEST : GTK_ANCHOR_EAST,
                                        "justification", GTK_JUSTIFY_RIGHT,
                                        "x", east_channel ? TOTAL_WIDTH (csource) + BORDER_PAD * 2. : -BORDER_PAD,
                                        "y", (y1 + y2) / 2.,
                                        "font", CHANNEL_FONT,
                                        "text", csource->show_hints ? label : "",
                                        NULL);
          g_object_connect (item,
                            "swapped_signal::destroy", channel_name_remove, csource,
                            NULL);
          gnome_canvas_text_set_zoom_size (GNOME_CANVAS_TEXT (item), FONT_HEIGHT);
          g_object_set_data_full (G_OBJECT (item), "hint_text", g_strdup (label), g_free);
          csource->channel_hints = g_slist_prepend (csource->channel_hints, item);
        }

      color1 += color1_delta;
      color2 += color2_delta;
      y1 = y2;
    }
}

static gboolean
bst_canvas_source_build_async (gpointer data)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (data);
  if (gnome_canvas_item_check_undisposed (item))
    {
      BstCanvasSource *csource = BST_CANVAS_SOURCE (item);
      GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
      
      /* keep in mind, that creation order affects stacking */

      /* add input and output channel items */
      if (!csource->built_ichannels)
        {
          csource->built_ichannels = TRUE;
          bst_canvas_source_build_channels (csource,
                                            TRUE,               /* input channels */
                                            0xffff00, 0x808000,	/* ichannels */
                                            0x00afff, 0x005880, /* jchannels */
                                            TRUE, FALSE);
          return TRUE;
        }
      if (!csource->built_ochannels)
        {
          csource->built_ochannels = TRUE;
          bst_canvas_source_build_channels (csource,
                                            FALSE,              /* output channels */
                                            0xff0000, 0x800000, /* ochannels */
                                            0, 0,               /* unused */
                                            TRUE, FALSE);
          return TRUE;
        }

      /* add icon to group */
      if (!csource->icon_item)
        {
          csource->icon_item = g_object_connect (gnome_canvas_item_new (group,
                                                                        GNOME_TYPE_CANVAS_PIXBUF,
                                                                        "x", ICON_X (csource),
                                                                        "y", ICON_Y (csource),
                                                                        "width", ICON_WIDTH (csource),
                                                                        "height", ICON_HEIGHT (csource),
                                                                        NULL),
                                                 "signal::destroy", gtk_widget_destroyed, &csource->icon_item,
                                                 "swapped_signal::event", bst_canvas_source_child_event, csource,
                                                 NULL);
          source_icon_changed (csource);
          return TRUE;
        }
      
      if (!csource->text)
        {
          /* add text item, invoke name_changed callback to setup the text value */
          guint ocolor = csource->source && bse_item_internal (csource->source) ? RGBA_INTERNAL : RGBA_BLACK;
          csource->text = gnome_canvas_item_new (group,
                                                 GNOME_TYPE_CANVAS_TEXT,
                                                 "fill_color_rgba", ocolor,
                                                 "anchor", GTK_ANCHOR_NORTH,
                                                 "justification", GTK_JUSTIFY_CENTER,
                                                 "x", TEXT_X (csource),
                                                 "y", TEXT_Y (csource),
                                                 "font", TEXT_FONT,
                                                 NULL);
          g_object_connect (csource->text,
                            "signal::destroy", gtk_widget_destroyed, &csource->text,
                            "swapped_signal::event", bst_canvas_source_child_event, csource,
                            NULL);
          gnome_canvas_text_set_zoom_size (GNOME_CANVAS_TEXT (csource->text), FONT_HEIGHT);
          source_name_changed (csource);
          return TRUE;
        }

      /* add input and output channel hints */
      if (!csource->built_ihints)
        {
          csource->built_ihints = TRUE;
          bst_canvas_source_build_channels (csource,
                                            TRUE,               /* input channels */
                                            0xffff00, 0x808000,	/* ichannels */
                                            0x00afff, 0x005880, /* jchannels */
                                            FALSE, TRUE);
          return TRUE;
        }
      if (!csource->built_ohints)
        {
          csource->built_ohints = TRUE;
          bst_canvas_source_build_channels (csource,
                                            FALSE,              /* output channels */
                                            0xff0000, 0x800000, /* ochannels */
                                            0, 0,               /* unused */
                                            FALSE, TRUE);
          return TRUE;
        }
    }
  GnomeCanvas *canvas = g_object_steal_data (item, "bst-workaround-canvas-ref");
  g_object_unref (item);
  if (canvas)
    g_object_unref (canvas);      /* canvases don't properly protect their items */
  return FALSE;
}

static void
bst_canvas_source_build (BstCanvasSource *csource)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  /* put an outer rectangle, make it transparent in aa mode,
   * so we can receive mouse events everywhere
   */
  GnomeCanvasItem *rect = gnome_canvas_item_new (group,
                                                 GNOME_TYPE_CANVAS_RECT,
                                                 "outline_color_rgba", RGBA_BLACK, /* covers buggy canvas lines */
                                                 "x1", 0.0,
                                                 "y1", 0.0,
                                                 "x2", TOTAL_WIDTH (csource),
                                                 "y2", TOTAL_HEIGHT (csource),
                                                 (GNOME_CANVAS_ITEM (csource)->canvas->aa
                                                  ? "fill_color_rgba"
                                                  : NULL), 0x00000000,
                                                 NULL);
  g_object_connect (rect,
                    "swapped_signal::event", bst_canvas_source_child_event, csource,
                    NULL);
  /* make sure no items are left over */
  while (csource->channel_items)
    gtk_object_destroy (csource->channel_items->data);
  while (csource->channel_hints)
    gtk_object_destroy (csource->channel_hints->data);
  csource->built_ichannels = FALSE;
  csource->built_ochannels = FALSE;
  csource->built_ihints = FALSE;
  csource->built_ohints = FALSE;
  /* asynchronously rebuild contents */
  GnomeCanvasItem *csource_item = GNOME_CANVAS_ITEM (csource);
  /* work around stale canvas pointers, see #340437 */
  g_object_set_data_full (csource_item, "bst-workaround-canvas-ref", g_object_ref (csource_item->canvas), g_object_unref);
  bst_background_handler2_add (bst_canvas_source_build_async, g_object_ref (csource), NULL);
}

static void
bst_canvas_source_changed (BstCanvasSource *csource)
{
  if (csource->source)
    {
      GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
      gdouble x = 0, y = 0;
      gnome_canvas_item_i2w (item, &x, &y);
      bse_object_set_parasite_coords (csource->source, x, y);
    }
}

static gboolean
bst_canvas_source_event (GnomeCanvasItem *item,
			 GdkEvent        *event)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (item);
  gboolean handled = FALSE;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (!csource->in_move && event->button.button == 2)
	{
	  GdkCursor *fleur = gdk_cursor_new (GDK_FLEUR);
	  if (gnome_canvas_item_grab (item,
                                      GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                                      fleur,
                                      event->button.time) == 0)
            {
              gdouble x = event->button.x, y = event->button.y;
              gnome_canvas_item_w2i (item, &x, &y);
              csource->move_dx = x;
              csource->move_dy = y;
              csource->in_move = TRUE;
              bse_item_group_undo (csource->source, "Move");
            }
	  gdk_cursor_destroy (fleur);
	  handled = TRUE;
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (csource->in_move)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (item, &x, &y);
	  gnome_canvas_item_move (item, x - csource->move_dx, y - csource->move_dy);
	  GNOME_CANVAS_NOTIFY (item);
	  handled = TRUE;
	}
      else
	{
	  guint channel;
	  const gchar *label = NULL, *prefix = NULL, *ident = NULL;

	  /* set i/o channel hints */
	  channel = bst_canvas_source_ichannel_at (csource, event->motion.x, event->motion.y);
	  if (channel != ~0)
	    {
	      label = bse_source_ichannel_label (csource->source, channel);
	      ident = bse_source_ichannel_ident (csource->source, channel);
	      prefix = _("Input");
	    }
	  else
	    {
	      channel = bst_canvas_source_ochannel_at (csource, event->motion.x, event->motion.y);
	      if (channel != ~0)
		{
		  label = bse_source_ochannel_label (csource->source, channel);
		  ident = bse_source_ochannel_ident (csource->source, channel);
		  prefix = _("Output");
		}
	    }
	  if (label)
	    gxk_status_printf (GXK_STATUS_IDLE_HINT, _("(Hint)"), "%s[%s]: %s", prefix, ident, label);
	  else
	    gxk_status_set (GXK_STATUS_IDLE_HINT, NULL, NULL);
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 2 && csource->in_move)
	{
          bse_item_ungroup_undo (csource->source);
	  csource->in_move = FALSE;
	  gnome_canvas_item_ungrab (item, event->button.time);
	  handled = TRUE;
	}
      break;
    default:
      break;
    }
  
  if (!handled && GNOME_CANVAS_ITEM_CLASS (bst_canvas_source_parent_class)->event)
    handled = GNOME_CANVAS_ITEM_CLASS (bst_canvas_source_parent_class)->event (item, event);
  
  return handled;
}

static gboolean
bst_canvas_source_child_event (BstCanvasSource *csource,
			       GdkEvent        *event,
			       GnomeCanvasItem *child)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
  GtkWidget *widget = GTK_WIDGET (item->canvas);
  gboolean handled = FALSE;
  
  csource = BST_CANVAS_SOURCE (item);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
      handled = TRUE;
      break;
    case GDK_LEAVE_NOTIFY:
      handled = TRUE;
      break;
    case GDK_KEY_PRESS:
      switch (event->key.keyval)
	{
	case 'L':
	case 'l':
	  gnome_canvas_item_lower_to_bottom (item);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'R':
	case 'r':
	  gnome_canvas_item_raise_to_top (item);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	}
      handled = TRUE;
      break;
    case GDK_KEY_RELEASE:
      handled = TRUE;
      break;
    default:
      break;
    }
  
  return handled;
}

static void
bst_canvas_source_class_init (BstCanvasSourceClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GnomeCanvasItemClass *canvas_item_class = GNOME_CANVAS_ITEM_CLASS (class);
  /* GnomeCanvasGroupClass *canvas_group_class = GNOME_CANVAS_GROUP_CLASS (class); */
  
  object_class->destroy = bst_canvas_source_destroy;
  canvas_item_class->event = bst_canvas_source_event;
  class->update_links = NULL;

  csource_signals[SIGNAL_UPDATE_LINKS] =
    gtk_signal_new ("update-links",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstCanvasSourceClass, update_links),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
}

/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
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
#ifndef __GXK_TEXT_TOOLS_H__
#define __GXK_TEXT_TOOLS_H__

#include        "gxkutils.h"

G_BEGIN_DECLS

/* --- text tools flags --- */
typedef enum /*< skip >*/
{
  GXK_SCROLL_TEXT_MONO		= 0 << 0,
  GXK_SCROLL_TEXT_SANS		= 1 << 0,
  GXK_SCROLL_TEXT_SERIF		= 2 << 0,
  GXK_SCROLL_TEXT_WRAP		= 1 << 2,
  GXK_SCROLL_TEXT_CENTER	= 1 << 3,
  GXK_SCROLL_TEXT_WIDGET_BG	= 1 << 4,
  GXK_SCROLL_TEXT_NAVIGATABLE	= 1 << 5,
  GXK_SCROLL_TEXT_EDITABLE	= 1 << 6,
  GXK_SCROLL_TEXT_HFIXED	= 1 << 7,
  GXK_SCROLL_TEXT_VFIXED	= 1 << 8
#define GXK_SCROLL_TEXT_WIDGET_LOOK	(GXK_SCROLL_TEXT_WRAP | GXK_SCROLL_TEXT_WIDGET_BG | GXK_SCROLL_TEXT_SANS)
} GxkScrollTextFlags;


/* --- text tools functions --- */
void		gxk_text_view_enter_browse_mode	(GtkTextView		*tview);
void		gxk_text_view_leave_browse_mode	(GtkTextView		*tview);
void		gxk_text_view_cursor_to_start	(GtkTextView		*tview);
void		gxk_text_view_cursor_to_end	(GtkTextView		*tview);
void		gxk_text_view_cursor_normal	(GtkTextView		*tview);
void		gxk_text_view_cursor_busy	(GtkTextView		*tview);
GtkWidget*	gxk_scroll_text_create		(GxkScrollTextFlags	 flags,
						 const gchar		*string);
GtkWidget*	gxk_scroll_text_create_for	(GxkScrollTextFlags	 flags,
						 GtkWidget              *parent);
void		gxk_scroll_text_set_index	(GtkWidget		*sctext,
						 const gchar		*uri);
void		gxk_scroll_text_display		(GtkWidget		*sctext,
						 const gchar		*uri);
void		gxk_scroll_text_enter		(GtkWidget		*sctext,
						 const gchar		*uri);
void		gxk_scroll_text_advance		(GtkWidget		*sctext,
						 const gchar		*uri);
void		gxk_scroll_text_rewind		(GtkWidget		*sctext);
void		gxk_scroll_text_set		(GtkWidget		*sctext,
						 const gchar		*string);
void		gxk_scroll_text_set_tsm		(GtkWidget		*sctext,
						 const gchar		*string);
void		gxk_scroll_text_clear		(GtkWidget		*sctext);
void		gxk_scroll_text_push_indent	(GtkWidget		*sctext);
void		gxk_scroll_text_append		(GtkWidget		*sctext,
						 const gchar		*string);
void		gxk_scroll_text_append_tsm	(GtkWidget		*sctext,
						 const gchar		*string);
void		gxk_scroll_text_append_file	(GtkWidget		*sctext,
						 const gchar    	*file_name);
void		gxk_scroll_text_append_file_tsm	(GtkWidget		*sctext,
						 const gchar    	*file_name);
void		gxk_scroll_text_aprintf		(GtkWidget		*sctext,
						 const gchar		*text_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void		gxk_scroll_text_pop_indent	(GtkWidget		*sctext);
GtkTextView*	gxk_scroll_text_get_text_view	(GtkWidget		*sctext);
void		gxk_text_add_tsm_path		(const gchar		*path);
void	gxk_text_buffer_init_custom			(void);
void	gxk_text_buffer_cursor_to_start			(GtkTextBuffer	*tbuffer);
void	gxk_text_buffer_cursor_to_end			(GtkTextBuffer	*tbuffer);
void	gxk_text_buffer_append_from_string		(GtkTextBuffer	*tbuffer,
							 gboolean	 parse_tsm,
							 guint		 indent_margin,
							 guint		 text_length,
							 const gchar	*text);
void	gxk_text_buffer_append_from_file		(GtkTextBuffer	*tbuffer,
							 gboolean	 parse_tsm,
							 guint		 indent_margin,
							 const gchar	*file_name);


/* --- special tag handlers --- */
typedef GtkWidget* (*GxkTextTextgetHandler)  (gpointer              user_data,
                                              const gchar          *element_name,
                                              const gchar         **attribute_names,
                                              const gchar         **attribute_values);
void    gxk_text_register_textget_handler    (const gchar          *element_name,
                                              GxkTextTextgetHandler handler,
                                              gpointer              user_data);
void    gxk_text_buffer_add_textgets_to_view (GtkTextBuffer        *tbuffer,
                                              GtkTextView          *tview);


G_END_DECLS

#endif /* __GXK_TEXT_TOOLS_H__ */

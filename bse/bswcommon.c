/* BSW - Bedevilled Sound Engine Wrapper
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
#include "bswcommon.h"

#include "../PKG_config.h"
#include "gslcommon.h"
#include "bseplugin.h"
#include "bseglue.h"
#include "bsescripthelper.h"
#include <string.h>


#define	ITER_ITEMS_PREALLOC	(8)


/* --- initialize scripts and plugins --- */
static void
handle_message (GString *gstring,
		gpointer captured)
{
  if (!captured && gstring->len)
    {
      g_message ("%s", gstring->str);
      g_string_set_size (gstring, 0);
    }
}

void
bsw_register_plugins (const gchar *path,
		      gboolean     verbose,
		      gchar      **messages,
		      void       (*callback) (gpointer     data,
					      const gchar *name),
		      gpointer     data)
{
  GSList *free_list, *list;
  GString *gstring = g_string_new (NULL);

  if (!path)
    path = BSE_PATH_PLUGINS;

  free_list = bse_plugin_dir_list_files (path);
  for (list = free_list; list; list = list->next)
    {
      gchar *string = list->data;
      const gchar *error;

      if (callback)
	callback (data, string);
      else if (verbose)
	g_string_printfa (gstring, "register plugin \"%s\"...", string);
      handle_message (gstring, messages);
      error = bse_plugin_check_load (string);
      if (error)
	g_string_printfa (gstring, "error encountered loading plugin \"%s\": %s", string, error);
      handle_message (gstring, messages);
      g_free (string);
    }
  g_slist_free (free_list);
  if (!free_list && verbose)
    {
      g_string_printfa (gstring, "strange, can't find any plugins, please check %s", path);
      handle_message (gstring, messages);
    }
  if (messages && gstring->len)
    *messages = g_string_free (gstring, FALSE);
  else
    {
      if (messages)
	*messages = NULL;
      g_string_free (gstring, TRUE);
    }
}

void
bsw_register_scripts (const gchar *path,
		      gboolean     verbose,
		      gchar      **messages,
		      void       (*callback) (gpointer     data,
					      const gchar *name),
		      gpointer     data)
{
  GSList *free_list, *list;
  GString *gstring = g_string_new (NULL);

  if (!path)
    path = BSW_PATH_SCRIPTS;

  free_list = bse_script_dir_list_files (path);
  for (list = free_list; list; list = list->next)
    {
      gchar *string = list->data;
      BseErrorType error;

      if (callback)
	callback (data, string);
      else if (verbose)
	g_string_printfa (gstring, "register script \"%s\"...", string);
      handle_message (gstring, messages);
      error = bse_script_file_register (string);
      if (error)
	g_string_printfa (gstring, "error encountered loading script \"%s\": %s", string, bse_error_blurb (error));
      handle_message (gstring, messages);
      g_free (string);
    }
  g_slist_free (free_list);
  if (!free_list && verbose)
    {
      g_string_printfa (gstring, "strange, can't find any scripts, please check %s", path);
      handle_message (gstring, messages);
    }
  if (messages && gstring->len)
    *messages = g_string_free (gstring, FALSE);
  else
    {
      if (messages)
	*messages = NULL;
      g_string_free (gstring, TRUE);
    }
}

gchar*
bsw_type_name_to_sname (const gchar *type_name)
{
  gchar *name = g_type_name_to_sname (type_name);

  if (name && name[0] == 'b' && name[1] == 's' && name[2] == 'e')
    name[2] = 'w';

  return name;
}

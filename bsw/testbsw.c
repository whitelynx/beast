/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2001 Tim Janik
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
#include        <bsw/bsw.h>

#define ASSERT(code)    do { if (code) ; else g_error ("failed to assert: %s", G_STRINGIFY (code)); } while (0)

int
main (int   argc,
      char *argv[])
{
  SfiProxy project;
  gint error;
  
  g_thread_init (NULL);

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);

  bsw_init (&argc, &argv, NULL);	// FIXME

  g_print ("server id: %lu\n", BSW_SERVER);
  ASSERT (BSW_SERVER > 0);

  bse_hello_world ();
  project = bse_server_use_new_project (BSW_SERVER, "test-project");
  g_print ("project id: %lu\n", project);
  ASSERT (project > BSW_SERVER);
  error = bse_project_restore_from_file (project, "/YOUbetterDONThaveTHIS");
  g_print ("load project result: %s\n", bse_error_blurb (error));
  ASSERT (error == BSE_ERROR_NOT_FOUND);

  bse_item_unuse (project);
  
  return 0;
}

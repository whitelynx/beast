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
#include	"bstgconfig.h"


// FIXME: reimplement beast config values and preferences

/* --- functions --- */
void
bst_globals_init (void)
{
}

void
bst_globals_set_rc_version (const gchar *rc_version)
{
  g_return_if_fail (bse_globals_locked () == FALSE);

  // g_free (bst_globals_current.rc_version);
  // bst_globals_current.rc_version = g_strdup (rc_version);
}

void
bst_globals_set_xkb_symbol (const gchar *xkb_symbol)
{
  g_return_if_fail (bse_globals_locked () == FALSE);

  // g_free (bst_globals_current.xkb_symbol);
  // bst_globals_current.xkb_symbol = g_strdup (xkb_symbol);
}

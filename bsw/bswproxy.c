/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
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
#include "bswproxy.h"

#include <bse/bsemain.h>
#include <bse/bseglue.h>	// FIXME
#include <bse/bseserver.h>
#include <gobject/gvaluecollector.h>


/* --- functions --- */
void
bsw_init (gint               *argc,
	  gchar             **argv[],
	  const BswLockFuncs *lock_funcs)
{
  static gboolean initialized = 0;
  BseLockFuncs lfuncs;

  g_return_if_fail (initialized++ == 0);

  if (lock_funcs)
    {
      g_return_if_fail (lock_funcs->lock != NULL);
      g_return_if_fail (lock_funcs->unlock != NULL);

      lfuncs.lock_data = lock_funcs->lock_data;
      lfuncs.lock = lock_funcs->lock;
      lfuncs.unlock = lock_funcs->unlock;
    }
  
  bse_init (argc, argv, lock_funcs ? &lfuncs : NULL);

  sfi_glue_context_push (bse_glue_context ("BSW-Client"));
}

SfiProxy
bsw_proxy_get_server (void)
{
  return BSE_OBJECT_ID (bse_server_get ());
}

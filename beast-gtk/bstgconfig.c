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


/* --- variables --- */
BstGlobalConfig   *bst_globals = NULL;
static GParamSpec *pspec_global_config = NULL;


/* --- functions --- */
void
bst_globals_init (void)
{
  BstGlobalConfig *gconfig;
  GValue *value;
  SfiRec *rec;

  g_return_if_fail (bst_globals == NULL);

  /* global config record description */
  pspec_global_config = sfi_pspec_rec ("global-config", NULL, NULL,
				       bst_global_config_fields, SFI_PARAM_DEFAULT);
  g_param_spec_ref (pspec_global_config);
  g_param_spec_sink (pspec_global_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_global_config, value);
  /* install global config */
  gconfig = bst_global_config_from_rec (rec);
  bst_globals = gconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
}

static BstGlobalConfig*
copy_gconfig (BstGlobalConfig *src_config)
{
  SfiRec *rec1 = bst_global_config_to_rec (src_config);
  SfiRec *rec2 = sfi_rec_copy_deep (rec1);
  BstGlobalConfig *gconfig;
  sfi_rec_unref (rec1);
  gconfig = bst_global_config_from_rec (rec2);
  sfi_rec_unref (rec2);
  return gconfig;
}

void
bst_globals_set_rc_version (const gchar *rc_version)
{
  BstGlobalConfig *gconfig, *oldconfig;

  g_return_if_fail (bse_globals_locked () == FALSE);

  gconfig = copy_gconfig (bst_globals);
  g_free (gconfig->rc_version);
  gconfig->rc_version = g_strdup (rc_version);
  oldconfig = bst_globals;
  bst_globals = gconfig;
  bst_global_config_free (oldconfig);
}

void
bst_globals_set_xkb_symbol (const gchar *xkb_symbol)
{
  BstGlobalConfig *gconfig, *oldconfig;

  g_return_if_fail (bse_globals_locked () == FALSE);

  gconfig = copy_gconfig (bst_globals);
  g_free (gconfig->xkb_symbol);
  gconfig->xkb_symbol = g_strdup (xkb_symbol);
  oldconfig = bst_globals;
  bst_globals = gconfig;
  bst_global_config_free (oldconfig);
}

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
BstGConfig        *bst_global_config = NULL;
static GParamSpec *pspec_global_config = NULL;


/* --- functions --- */
void
_bst_gconfig_init (void)
{
  BstGConfig *gconfig;
  GValue *value;
  SfiRec *rec;

  g_return_if_fail (bst_global_config == NULL);

  /* global config record description */
  pspec_global_config = sfi_pspec_rec ("global-config", NULL, NULL,
				       bst_gconfig_fields, SFI_PARAM_DEFAULT);
  g_param_spec_ref (pspec_global_config);
  g_param_spec_sink (pspec_global_config);
  /* create empty config record */
  rec = sfi_rec_new ();
  value = sfi_value_rec (rec);
  /* fill out missing values with defaults */
  g_param_value_validate (pspec_global_config, value);
  /* install global config */
  gconfig = bst_gconfig_from_rec (rec);
  bst_global_config = gconfig;
  /* cleanup */
  sfi_value_free (value);
  sfi_rec_unref (rec);
}

GParamSpec*
bst_gconfig_pspec (void)
{
  return pspec_global_config;
}

static BstGConfig*
copy_gconfig (BstGConfig *src_config)
{
  SfiRec *rec = bst_gconfig_to_rec (src_config);
  BstGConfig *gconfig = bst_gconfig_from_rec (rec);
  sfi_rec_unref (rec);
  return gconfig;
}

static void
set_gconfig (BstGConfig *gconfig)
{
  BstGConfig *oldconfig = bst_global_config;
  bst_global_config = gconfig;
  bst_gconfig_free (oldconfig);
  {
    SfiRec *prec = bst_gconfig_to_rec (bst_global_config);
    GValue *v = sfi_value_rec (prec);
    GString *gstring = g_string_new (NULL);
    sfi_value_store_param (v, gstring, pspec_global_config, 2);
    g_print ("CONFIG:\n%s\n", gstring->str);
    g_string_free (gstring, TRUE);
    sfi_value_free (v);
    sfi_rec_unref (prec);
  }
}

void
bst_gconfig_apply (SfiRec *rec)
{
  SfiRec *vrec;
  BstGConfig *gconfig;

  g_return_if_fail (rec != NULL);

  vrec = sfi_rec_copy_deep (rec);
  sfi_rec_validate (vrec, sfi_pspec_get_rec_fields (pspec_global_config));
  gconfig = bst_gconfig_from_rec (vrec);
  sfi_rec_unref (vrec);
  set_gconfig (gconfig);
}

void
bst_gconfig_set_rc_version (const gchar *rc_version)
{
  BstGConfig *gconfig;

  g_return_if_fail (bse_globals_locked () == FALSE);

  gconfig = copy_gconfig (bst_global_config);
  g_free (gconfig->rc_version);
  gconfig->rc_version = g_strdup (rc_version);
  set_gconfig (gconfig);
}

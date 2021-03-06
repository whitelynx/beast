/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_BSE_UTILS_H__
#define __BST_BSE_UTILS_H__

#include <sfi/sfi.h> /* needed by bstgenbseapi.h */

G_BEGIN_DECLS

#include "bstgenbseapi.h" /* include this within extern "C" */

/* --- BSE utilities --- */
#define BSE_SERVER                              (1 /* HACK */ )
#define bse_proxy_set_property                  sfi_glue_proxy_set_property
#define bse_proxy_get_property                  sfi_glue_proxy_get_property
#define bse_proxy_set                           sfi_glue_proxy_set
#define bse_proxy_get                           sfi_glue_proxy_get
#define bse_proxy_get_pspec                     sfi_glue_proxy_get_pspec
#define bse_proxy_list_properties               sfi_glue_proxy_list_properties
#define bse_proxy_disconnect                    sfi_glue_proxy_disconnect
#define bse_proxy_connect                       sfi_glue_proxy_connect
#define bse_proxy_pending                       sfi_glue_proxy_pending
#define bse_proxy_is_a                          sfi_glue_proxy_is_a
#define bse_proxy_get_qdata                     sfi_glue_proxy_get_qdata
#define bse_proxy_set_qdata_full                sfi_glue_proxy_set_qdata_full
#define bse_proxy_steal_qdata                   sfi_glue_proxy_steal_qdata
#define bse_proxy_set_qdata(p,q,d)              bse_proxy_set_qdata_full ((p), (q), (d), NULL)
#define bse_proxy_set_data(p,n,d)               bse_proxy_set_qdata ((p), g_quark_from_string (n), (d))
#define bse_proxy_get_data(p,n)                 bse_proxy_get_qdata ((p), g_quark_try_string (n))
#define bse_proxy_steal_data(p,n)               bse_proxy_steal_qdata ((p), g_quark_try_string (n))
#define bse_proxy_set_data_full(p,n,d,f)        bse_proxy_set_qdata_full ((p), g_quark_from_string (n), (d), (f))

/* --- BEAST utilities --- */
BseErrorType    bst_project_restore_from_file   (SfiProxy        project,
                                                 const gchar    *file_name,
                                                 bool            apply_project_file_name,
						 bool            preserve_non_dirty);
BseErrorType    bst_project_import_midi_file    (SfiProxy        project,
                                                 const gchar    *file_name);
const gchar*    bst_procedure_get_title         (const gchar    *procedure);

G_END_DECLS

#endif /* __BST_BSE_UTILS_H__ */

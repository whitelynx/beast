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
 *
 * bswcommon.h: BSW Types used also by BSE
 */
#ifndef __BSW_COMMON_H__
#define __BSW_COMMON_H__

#include	<sfi/sfi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- initialize scripts and plugins --- */
void	bsw_register_plugins	(const gchar	*path,
				 gboolean	 verbose,
				 gchar	       **messages,
				 void          (*callback) (gpointer     data,
							    const gchar *name),
				 gpointer	 data);
void	bsw_register_scripts	(const gchar	*path,
				 gboolean	 verbose,
				 gchar	       **messages,
				 void          (*callback) (gpointer     data,
							    const gchar *name),
				 gpointer	 data);


/* --- missing GLib --- */
gchar*  bsw_type_name_to_sname          (const gchar    *type_name);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_COMMON_H__ */

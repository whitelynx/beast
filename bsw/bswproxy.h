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
#ifndef __BSW_PROXY_H__
#define __BSW_PROXY_H__

#include        <bse/bswcommon.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	BSW_SERVER		(bsw_proxy_get_server ())
#define	BSE_SERVER		(bsw_proxy_get_server ())

typedef struct
{
  gpointer        lock_data;
  void          (*lock)         (gpointer lock_data);
  void          (*unlock)       (gpointer lock_data);
} BswLockFuncs;

void		bsw_init			(gint			*argc,
						 gchar		       **argv[],
						 const BswLockFuncs	*lock_funcs);
SfiProxy	bsw_proxy_get_server		(void)	G_GNUC_CONST;

void		bse_proxy_set			(SfiProxy		 proxy,
						 const gchar		*prop,
						 ...);
void		bse_proxy_get			(SfiProxy		 proxy,
						 const gchar		*prop,
						 ...);
#define	bse_proxy_get_pspec	sfi_proxy_get_pspec
GParamSpec*	sfi_proxy_get_pspec		(SfiProxy		 proxy,
						 const gchar		*name);
typedef struct {
  guint   n_strings;
  gchar **strings;
} SfiStringSeq;
#define	bse_proxy_list_properties	sfi_proxy_list_properties
SfiStringSeq*	sfi_proxy_list_properties	(SfiProxy		 proxy,
						 const gchar		*first_ancestor,
						 const gchar		*last_ancestor);
#define	bse_proxy_disconnect	sfi_proxy_disconnect
void		sfi_proxy_disconnect	(SfiProxy	 proxy,
					 const gchar	*signal,
					 ...);
#define	bse_proxy_connect	sfi_proxy_connect
void		sfi_proxy_connect	(SfiProxy	 proxy,
					 const gchar	*signal,
					 ...);
#define	bse_proxy_is_a	sfi_proxy_is_a
gboolean	sfi_proxy_is_a		(SfiProxy	 proxy,
					 const gchar	*type);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_PROXY_H__ */

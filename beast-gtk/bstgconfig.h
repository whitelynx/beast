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
#ifndef __BST_GLOBALS_H__
#define __BST_GLOBALS_H__

#include	"bstutils.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BstGlobals - configurable defaults --- */
#define BST_XKB_FORCE_QUERY		(bst_globals->xkb_force_query)
#define	BST_RC_VERSION			(bst_globals->rc_version)
#define BST_XKB_SYMBOL			(bst_globals->xkb_symbol)
#define BST_DISABLE_ALSA		(bst_globals->disable_alsa)
#define BST_TAB_WIDTH			(bst_globals->tab_width)
#define BST_SNET_ANTI_ALIASED		(bst_globals->snet_anti_aliased)
#define BST_SNET_EDIT_FALLBACK		(bst_globals->snet_edit_fallback)
#define BST_SNET_SWAP_IO_CHANNELS	(bst_globals->snet_swap_io_channels)
#define BST_SAMPLE_SWEEP		(bst_globals->sample_sweep)
#define BST_PE_KEY_FOCUS_UNSELECTS	(bst_globals->pe_key_focus_unselects)

extern BstGlobalConfig *bst_globals;


/* --- prototypes --- */
void	     bst_globals_init           (void);
void	     bst_globals_set_rc_version	(const gchar *rc_version);
void	     bst_globals_set_xkb_symbol	(const gchar *xkb_symbol);

typedef struct _BstGConfig BstGConfig;
BstGConfig*	bst_gconfig_new		(void);
void		bst_gconfig_revert	(BstGConfig	*gconfig);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_GLOBALS_H__ */

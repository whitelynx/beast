/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef	__BSE_WAVE_REPO_H__
#define	__BSE_WAVE_REPO_H__

#include	<bse/bsesuper.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_WAVE_REPO	        (BSE_TYPE_ID (BseWaveRepo))
#define BSE_WAVE_REPO(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_REPO, BseWaveRepo))
#define BSE_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))
#define BSE_IS_WAVE_REPO(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_REPO))
#define BSE_IS_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_REPO))
#define BSE_WAVE_REPO_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))


/* --- BseWaveRepo object --- */
struct _BseWaveRepo
{
  BseSuper	 parent_object;

  GList		*waves;
};
struct _BseWaveRepoClass
{
  BseSuperClass parent_class;
};


/* --- prototypes --- */
  

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_WAVE_REPO_H__ */

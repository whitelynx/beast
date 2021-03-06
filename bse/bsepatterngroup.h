/* BSE - Better Sound Engine
 * Copyright (C) 2000-2002 Tim Janik
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
 *
 * bsepatterngroup.h: patternary group container
 */
#ifndef __BSE_PATTERN_GROUP_H__
#define __BSE_PATTERN_GROUP_H__

#include        <bse/bsepattern.h>


/* --- object type macros --- */
#define	BSE_TYPE_PATTERN_GROUP		    (BSE_TYPE_ID (BsePatternGroup))
#define BSE_PATTERN_GROUP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PATTERN_GROUP, BsePatternGroup))
#define BSE_PATTERN_GROUP_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PATTERN_GROUP, BsePatternGroupClass))
#define BSE_IS_PATTERN_GROUP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PATTERN_GROUP))
#define BSE_IS_PATTERN_GROUP_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PATTERN_GROUP))
#define BSE_PATTERN_GROUP_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PATTERN_GROUP, BsePatternGroupClass))


/* --- BsePatternGroup object --- */
typedef struct _BsePatternGroupEntry BsePatternGroupEntry;
struct _BsePatternGroup
{
  BseItem		 parent_object;

  guint			 pattern_count;
  guint			 n_entries;
  BsePatternGroupEntry	*entries;
};
struct _BsePatternGroupClass
{
  BseItemClass parent_class;
};
struct _BsePatternGroupEntry
{
  BsePattern *pattern;
};


/* --- prototypes --- */
void	    bse_pattern_group_insert_pattern	(BsePatternGroup	*pgroup,
						 BsePattern		*pattern,
						 gint                    position);
void	    bse_pattern_group_remove_pattern	(BsePatternGroup	*pgroup,
						 BsePattern		*pattern);
void	    bse_pattern_group_remove_entry	(BsePatternGroup	*pgroup,
						 gint			 position);
void	    bse_pattern_group_clone_contents	(BsePatternGroup	*pgroup,
						 BsePatternGroup	*src_pgroup);
BsePattern* bse_pattern_group_get_nth_pattern	(BsePatternGroup	*pgroup,
						 gint			 index);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PATTERN_GROUP_H__ */

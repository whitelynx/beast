/* BSE Engine - Flow module operation engine
 * Copyright (C) 2001, 2002, 2003, 2004 Tim Janik
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
#ifndef __BSE_ENGINE_SCHEDULE_H__
#define __BSE_ENGINE_SCHEDULE_H__

#include <bse/bseenginenode.h>

G_BEGIN_DECLS

typedef struct
{
  EngineNode *last;	/* resolve node */
  SfiRing    *nodes;	/* of type EngineNode* */
  guint       seen_deferred_node : 1;
} EngineCycle;
typedef struct
{
  guint    leaf_level;
  SfiRing *cycles;	/* of type Cycle* */
  SfiRing *cycle_nodes;	/* of type EngineNode* */
} EngineQuery;
struct _EngineSchedule
{
  guint     n_items;
  guint     leaf_levels;
  SfiRing **nodes;	/* EngineNode* */
  SfiRing **cycles;	/* SfiRing* */
  guint	    secured : 1;
  guint	    in_pqueue : 1;
  guint	    cur_leaf_level;
  SfiRing  *cur_node;
  SfiRing  *cur_cycle;
  SfiRing  *vnodes;	/* virtual modules */
};
#define	BSE_ENGINE_SCHEDULE_NONPOPABLE(schedule)        ((schedule)->cur_leaf_level >= (schedule)->leaf_levels)


/* --- MasterThread --- */
EngineSchedule*	_engine_schedule_new		(void);
void		_engine_schedule_clear		(EngineSchedule	*schedule);
void		_engine_schedule_destroy	(EngineSchedule	*schedule);
void		_engine_schedule_consumer_node	(EngineSchedule	*schedule,
						 EngineNode	*node);
void		_engine_schedule_secure		(EngineSchedule	*schedule);
EngineNode*	_engine_schedule_pop_node	(EngineSchedule	*schedule);
SfiRing*	_engine_schedule_pop_cycle	(EngineSchedule	*schedule);
void		_engine_schedule_restart	(EngineSchedule	*schedule);
void		_engine_schedule_unsecure	(EngineSchedule	*schedule);

G_END_DECLS

#endif /* __BSE_ENGINE_SCHEDULE_H__ */

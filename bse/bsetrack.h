/* BSE - Better Sound Engine
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BSE_TRACK_H__
#define __BSE_TRACK_H__

#include <bse/bseitem.h>
#include <bse/bsesnet.h>
#include <bse/bsecontextmerger.h>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_TRACK		    (BSE_TYPE_ID (BseTrack))
#define BSE_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_TRACK, BseTrack))
#define BSE_TRACK_CLASS(class)	    (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_TRACK, BseTrackClass))
#define BSE_IS_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_TRACK))
#define BSE_IS_TRACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_TRACK))
#define BSE_TRACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_TRACK, BseTrackClass))


/* --- BseTrack --- */
typedef struct {
  guint    tick;
  guint    id;
  BsePart *part;
} BseTrackEntry;
struct _BseTrack
{
  BseContextMerger parent_instance;

  guint            channel_id;
  guint		   max_voices;
  BseSNet	  *snet;
  BseSNet         *pnet;

  /* wave synthesis */
  BseWave	  *wave;
  BseSNet	  *wnet;

  /* playback intergration */
  BseSource       *sub_synth;
  BseSource       *voice_input;
  BseSource       *voice_switch;
  BseSource       *postprocess;

  SfiRing         *bus_outputs; /* maintained by bsebus.[hc] */

  /* fields protected by sequencer mutex */
  guint		   n_entries_SL : 30;
  guint		   muted_SL : 1;
  BseTrackEntry	  *entries_SL;
  guint            midi_channel_SL;
  gboolean	   track_done_SL;
};
struct _BseTrackClass
{
  BseContextMergerClass parent_class;
};


/* --- prototypes -- */
void	bse_track_add_modules		(BseTrack		*self,
					 BseContainer		*container,
                                         BseMidiReceiver        *midi_receiver);
void	bse_track_remove_modules	(BseTrack		*self,
					 BseContainer		*container);
void	bse_track_clone_voices		(BseTrack		*self,
					 BseSNet		*snet,
					 guint			 context,
                                         BseMidiContext          mcontext,
					 BseTrans		*trans);
BseSource*       bse_track_get_output   (BseTrack               *self);
guint        	 bse_track_get_last_tick(BseTrack		*self);
guint        	 bse_track_insert_part	(BseTrack		*self,
					 guint			 tick,
					 BsePart		*part);
void		 bse_track_remove_tick	(BseTrack		*self,
					 guint			 tick);
BseTrackPartSeq* bse_track_list_parts	(BseTrack		*self);
BseTrackPartSeq* bse_track_list_part	(BseTrack		*self,
                                         BsePart                *part);
gboolean	 bse_track_find_part	(BseTrack		*self,
					 BsePart		*part,
					 guint			*start_p);
BseTrackEntry*	 bse_track_lookup_tick	(BseTrack		*self,
					 guint			 tick);
BseTrackEntry*   bse_track_find_link    (BseTrack               *self,
                                         guint                   id);
BsePart*	 bse_track_get_part_SL	(BseTrack		*self,
					 guint			 tick,
					 guint			*start,
					 guint			*next);

G_END_DECLS

#endif /* __BSE_TRACK_H__ */

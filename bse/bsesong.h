/* BSE - Better Sound Engine
 * Copyright (C) 1997-2003 Tim Janik
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
#ifndef __BSE_SONG_H__
#define __BSE_SONG_H__

#include        <bse/bsesnet.h>


G_BEGIN_DECLS


/* --- BSE type macros --- */
#define BSE_TYPE_SONG              (BSE_TYPE_ID (BseSong))
#define BSE_SONG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SONG, BseSong))
#define BSE_SONG_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SONG, BseSongClass))
#define BSE_IS_SONG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SONG))
#define BSE_IS_SONG_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SONG))
#define BSE_SONG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SONG, BseSongClass))


/* --- BseSong object --- */
typedef struct {
  BseSource *constant;
  BseSource *sub_synth;
} BseSongVoice;
struct _BseSong
{
  BseSNet           parent_instance;
  
  guint		    tpqn;		/* ticks per querter note */
  guint		    numerator;
  guint		    denominator;
  gfloat            bpm;

  BseMusicalTuningType musical_tuning;
  
  SfiRing          *parts;              /* of type BsePart* */
  SfiRing          *busses;             /* of type BseBus* */
  BseBus           *solo_bus;

  BseSource	   *postprocess;
  BseSource	   *output;

  BseSNet          *pnet;

  /* song position pointer */
  SfiInt	    last_position;
  guint		    position_handler;

  BseMidiReceiver  *midi_receiver_SL;

  /* fields protected by sequencer mutex */
  gdouble	    tpsi_SL;		/* ticks per stamp increment (sample) */
  SfiRing	   *tracks_SL;		/* of type BseTrack* */
  /* sequencer stuff */
  guint64           sequencer_start_request_SL;
  guint64           sequencer_start_SL; /* playback start */
  guint64           sequencer_done_SL;
  gdouble	    delta_stamp_SL;	/* start + delta_stamp => tick */
  guint		    tick_SL;		/* tick at stamp_SL */
  guint             sequencer_owns_refcount_SL : 1;
  guint             sequencer_underrun_detected_SL : 1;
  guint		    loop_enabled_SL : 1;
  SfiInt	    loop_left_SL;	/* left loop tick */
  SfiInt	    loop_right_SL;	/* left loop tick */
};
struct _BseSongClass
{
  BseSNetClass parent_class;
};


/* --- prototypes --- */
BseSong*	bse_song_lookup			(BseProject	*project,
						 const gchar	*name);
void		bse_song_stop_sequencing_SL	(BseSong	*self);
void		bse_song_get_timing		(BseSong	*self,
						 guint		 tick,
						 BseSongTiming	*timing);
void		bse_song_timing_get_default	(BseSongTiming	*timing);
BseSource*      bse_song_create_summation       (BseSong        *self);
BseBus*         bse_song_find_master            (BseSong        *self);
BseSource*      bse_song_ensure_master          (BseSong        *self);
void            bse_song_set_solo_bus           (BseSong        *self,
                                                 BseBus         *bus);

G_END_DECLS

#endif /* __BSE_SONG_H__ */

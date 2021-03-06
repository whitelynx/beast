/* BSE - Better Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#ifndef __BSE_MIDI_VOICE_H__
#define __BSE_MIDI_VOICE_H__

#include <bse/bsesource.h>
#include <bse/bsesnet.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_MIDI_VOICE_INPUT	        (BSE_TYPE_ID (BseMidiVoiceInput))
#define BSE_MIDI_VOICE_INPUT(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInput))
#define BSE_MIDI_VOICE_INPUT_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInputClass))
#define BSE_IS_MIDI_VOICE_INPUT(object)	        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_VOICE_INPUT))
#define BSE_IS_MIDI_VOICE_INPUT_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_VOICE_INPUT))
#define BSE_MIDI_VOICE_INPUT_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_VOICE_INPUT, BseMidiVoiceInputClass))
#define BSE_TYPE_MIDI_VOICE_SWITCH	        (BSE_TYPE_ID (BseMidiVoiceSwitch))
#define BSE_MIDI_VOICE_SWITCH(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitch))
#define BSE_MIDI_VOICE_SWITCH_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitchClass))
#define BSE_IS_MIDI_VOICE_SWITCH(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_MIDI_VOICE_SWITCH))
#define BSE_IS_MIDI_VOICE_SWITCH_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_MIDI_VOICE_SWITCH))
#define BSE_MIDI_VOICE_SWITCH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_MIDI_VOICE_SWITCH, BseMidiVoiceSwitchClass))


/* --- object structures --- */
typedef struct _BseMidiVoiceInput  BseMidiVoiceInput;
typedef struct _BseMidiVoiceSwitch BseMidiVoiceSwitch;
typedef struct _BseSourceClass     BseMidiVoiceInputClass;
typedef struct _BseSourceClass     BseMidiVoiceSwitchClass;
struct _BseMidiVoiceInput
{
  BseSource	parent_object;

  BseMidiVoiceSwitch *voice_switch;
};
struct _BseMidiVoiceSwitch
{
  BseSource      parent_object;
  guint		 midi_channel;
  GSList        *midi_voices;
};


/* --- prototypes --- */
void           bse_midi_voice_switch_set_midi_channel (BseMidiVoiceSwitch *self,
                                                       guint               midi_channel);
BseMidiContext bse_midi_voice_switch_ref_poly_voice   (BseMidiVoiceSwitch *self,
                                                       guint               context_handle,
                                                       BseTrans           *trans);
BseMidiContext bse_midi_voice_switch_peek_poly_voice  (BseMidiVoiceSwitch *self,
                                                       guint               context_handle);
void           bse_midi_voice_switch_unref_poly_voice (BseMidiVoiceSwitch *self,
                                                       guint               context_handle,
                                                       BseTrans           *trans);
void           bse_midi_voice_input_set_voice_switch  (BseMidiVoiceInput  *self,
                                                       BseMidiVoiceSwitch *voice_switch);


/* --- channels --- */
enum
{
  BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY,
  BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH
};
enum
{
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
  BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT
};
enum
{
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT,
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT,
  BSE_MIDI_VOICE_SWITCH_OCHANNEL_DISCONNECT
};

G_END_DECLS

#endif /* __BSE_MIDI_VOICE_H__ */

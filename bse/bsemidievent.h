/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-2002 Tim Janik
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
#ifndef __BSE_MIDI_EVENT_H__
#define __BSE_MIDI_EVENT_H__

#include        <bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- MIDI constants --- */
#define	BSE_MIDI_MAX_CHANNELS		(16)


/* --- MIDI event types --- */
typedef enum
{
  /* channel voice messages */
  BSE_MIDI_NOTE_OFF             = 0x80,         /* note, velocity */
  BSE_MIDI_NOTE_ON              = 0x90,         /* note, velocity */
  BSE_MIDI_KEY_PRESSURE         = 0xA0,         /* note, intensity */
  BSE_MIDI_CONTROL_CHANGE       = 0xB0,         /* ctl-nr, value */
  BSE_MIDI_PROGRAM_CHANGE       = 0xC0,         /* prg-nr */
  BSE_MIDI_CHANNEL_PRESSURE     = 0xD0,         /* intensity */
  BSE_MIDI_PITCH_BEND           = 0xE0,         /* 7lsb, 7msb */
  /* system common messages */
  BSE_MIDI_SYS_EX               = 0xF0,         /* data... */
  BSE_MIDI_SONG_POINTER         = 0xF2,         /* p1, p2 */
  BSE_MIDI_SONG_SELECT          = 0xF3,         /* song-nr */
  BSE_MIDI_TUNE                 = 0xF6,
  BSE_MIDI_END_EX               = 0xF7,
  /* system realtime messages */
  BSE_MIDI_TIMING_CLOCK         = 0xF8,
  BSE_MIDI_SONG_START           = 0xFA,
  BSE_MIDI_SONG_CONTINUE        = 0xFB,
  BSE_MIDI_SONG_STOP            = 0xFC,
  BSE_MIDI_ACTIVE_SENSING       = 0xFE,
  BSE_MIDI_SYSTEM_RESET         = 0xFF,
  /* internally used extra events */
  BSE_MIDI_X_CONTINUOUS_CHANGE  = 512
} BseMidiEventType;


/* --- BSE MIDI Event --- */
#define BSE_TYPE_MIDI_EVENT	(bse_midi_event_get_type ())
typedef struct
{
  BseMidiEventType status;
  guint            channel;     /* 0 .. 15 if valid */
  guint64          tick_stamp;  /* GSL tick stamp */
  union {
    struct {
      gfloat  frequency;
      gfloat  velocity;         /* or intensity: 0..+1 */
    }       note;
    struct {
      guint   control;          /* 0..0x7f */
      gfloat  value;            /* -1..+1 */
    }       control;
    guint   program;            /* 0..0x7f */
    gfloat  intensity;          /* 0..+1 */
    gfloat  pitch_bend;         /* -1..+1 */
    struct {
      guint   n_bytes;
      guint8 *bytes;
    }       sys_ex;
    guint   song_pointer;       /* 0..0x3fff */
    guint   song_number;        /* 0..0x7f */
  } data;
} BseMidiEvent;


/* --- API --- */
GType		 bse_midi_event_get_type	(void);	/* boxed */
void		 bse_midi_free_event		(BseMidiEvent	  *event);
BseMidiEvent*	 bse_midi_event_note_on		(guint		   midi_channel,
						 guint64	   tick_stamp,
						 gfloat		   frequency,
						 gfloat		   velocity);
BseMidiEvent*	 bse_midi_event_note_off	(guint		   midi_channel,
						 guint64	   tick_stamp,
						 gfloat		   frequency);
BseMidiEvent*    bse_midi_event_signal          (guint             midi_channel,
                                                 guint64           tick_stamp,
                                                 BseMidiSignalType signal_type,
                                                 gfloat            value);


/* --- MIDI Signals --- */
#if 0
typeNOTdef enum	/*< prefix=BSE_MIDI_SIGNAL >*/  /* FIXME: sync to bserecords.sfidl */
{
  /* special cased signals */
  BSE_MIDI_SIGNAL_PROGRAM	= 1,	/*< nick=Program Change >*/		/* 7bit */
  BSE_MIDI_SIGNAL_PRESSURE,		/*< nick=Channel Pressure >*/		/* 7bit */
  BSE_MIDI_SIGNAL_PITCH_BEND,		/*< nick=Pitch Bend >*/			/* 14bit */
  BSE_MIDI_SIGNAL_VELOCITY,             /*< nick=Note Velocity >*/
  BSE_MIDI_SIGNAL_FINE_TUNE,            /*< nick=Note Fine Tune >*/
  /* 14bit, continuous controls */
  BSE_MIDI_SIGNAL_CONTINUOUS_0	= 64,	/*< nick=Bank Select >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_1,		/*< nick=Modulation Wheel >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_2,		/*< nick=Breath Control >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_3,		/*< nick=Continuous 3 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_4,		/*< nick=Foot Controller >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_5,		/*< nick=Portamento Time >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_6,		/*< nick=Data Entry >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_7,		/*< nick=Volume >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_8,		/*< nick=Balance >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_9,		/*< nick=Continuous 9 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_10,	/*< nick=Panorama >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_11,	/*< nick=Expression >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_12,	/*< nick=Effect Control 1 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_13,	/*< nick=Effect Control 2 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_14,	/*< nick=Continuous 14 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_15,	/*< nick=Continuous 15 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_16,	/*< nick=General Purpose Controller 1 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_17,	/*< nick=General Purpose Controller 2 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_18,	/*< nick=General Purpose Controller 3 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_19,	/*< nick=General Purpose Controller 4 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_20,	/*< nick=Continuous 20 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_21,	/*< nick=Continuous 21 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_22,	/*< nick=Continuous 22 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_23,	/*< nick=Continuous 23 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_24,	/*< nick=Continuous 24 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_25,	/*< nick=Continuous 25 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_26,	/*< nick=Continuous 26 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_27,	/*< nick=Continuous 27 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_28,	/*< nick=Continuous 28 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_29,	/*< nick=Continuous 29 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_30,	/*< nick=Continuous 30 >*/
  BSE_MIDI_SIGNAL_CONTINUOUS_31,	/*< nick=Continuous 31 >*/
  /* 14bit, special cased signals */
  BSE_MIDI_SIGNAL_CONSTANT_HIGH	= 96,		/*< nick=Constant HIGH >*/
  BSE_MIDI_SIGNAL_CONSTANT_CENTER,		/*< nick=Constant CENTER >*/
  BSE_MIDI_SIGNAL_CONSTANT_LOW,			/*< nick=Constant LOW >*/
  BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_CENTER,	/*< nick=Constant Negative CENTER >*/
  BSE_MIDI_SIGNAL_CONSTANT_NEGATIVE_HIGH,	/*< nick=Constant Negative HIGH >*/
  BSE_MIDI_SIGNAL_PARAMETER,			/*< nick=Registered Parameter >*/
  BSE_MIDI_SIGNAL_NON_PARAMETER,		/*< nick=Non-Registered Parameter >*/
  /* 7bit, literal channel controls, MSB values */
  BSE_MIDI_SIGNAL_CONTROL_0	= 128,	/*< nick=Control 0 Bank Select MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_1,		/*< nick=Control 1 Modulation Wheel MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_2,		/*< nick=Control 2 Breath Control MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_3,
  BSE_MIDI_SIGNAL_CONTROL_4,		/*< nick=Control 4 Foot Controller MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_5,		/*< nick=Control 5 Portamento Time MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_6,		/*< nick=Control 6 Data Entry MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_7,		/*< nick=Control 7 Volume MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_8,		/*< nick=Control 8 Balance MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_9,
  BSE_MIDI_SIGNAL_CONTROL_10,		/*< nick=Control 10 Panorama MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_11,		/*< nick=Control 11 Expression MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_12,		/*< nick=Control 12 Effect Control 1 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_13,		/*< nick=Control 13 Effect Control 2 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_14,
  BSE_MIDI_SIGNAL_CONTROL_15,
  BSE_MIDI_SIGNAL_CONTROL_16,		/*< nick=Control 16 General Purpose Controller 1 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_17,		/*< nick=Control 17 General Purpose Controller 2 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_18,		/*< nick=Control 18 General Purpose Controller 3 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_19,		/*< nick=Control 19 General Purpose Controller 4 MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_20,
  BSE_MIDI_SIGNAL_CONTROL_21,
  BSE_MIDI_SIGNAL_CONTROL_22,
  BSE_MIDI_SIGNAL_CONTROL_23,
  BSE_MIDI_SIGNAL_CONTROL_24,
  BSE_MIDI_SIGNAL_CONTROL_25,
  BSE_MIDI_SIGNAL_CONTROL_26,
  BSE_MIDI_SIGNAL_CONTROL_27,
  BSE_MIDI_SIGNAL_CONTROL_28,
  BSE_MIDI_SIGNAL_CONTROL_29,
  BSE_MIDI_SIGNAL_CONTROL_30,
  BSE_MIDI_SIGNAL_CONTROL_31,
  /* 7bit, literal channel controls, LSB values */
  BSE_MIDI_SIGNAL_CONTROL_32,		/*< nick=Control 32 Bank Select LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_33,		/*< nick=Control 33 Modulation Wheel LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_34,		/*< nick=Control 34 Breath Control LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_35,
  BSE_MIDI_SIGNAL_CONTROL_36,		/*< nick=Control 36 Foot Controller LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_37,		/*< nick=Control 37 Portamento Time LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_38,		/*< nick=Control 38 Data Entry LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_39,		/*< nick=Control 39 Volume LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_40,		/*< nick=Control 40 Balance LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_41,
  BSE_MIDI_SIGNAL_CONTROL_42,		/*< nick=Control 42 Panorama LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_43,		/*< nick=Control 43 Expression LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_44,		/*< nick=Control 44 Effect Control 1 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_45,		/*< nick=Control 45 Effect Control 2 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_46,
  BSE_MIDI_SIGNAL_CONTROL_47,
  BSE_MIDI_SIGNAL_CONTROL_48,		/*< nick=Control 48 General Purpose Controller 1 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_49,		/*< nick=Control 49 General Purpose Controller 2 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_50,		/*< nick=Control 50 General Purpose Controller 3 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_51,		/*< nick=Control 51 General Purpose Controller 4 LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_52,
  BSE_MIDI_SIGNAL_CONTROL_53,
  BSE_MIDI_SIGNAL_CONTROL_54,
  BSE_MIDI_SIGNAL_CONTROL_55,
  BSE_MIDI_SIGNAL_CONTROL_56,
  BSE_MIDI_SIGNAL_CONTROL_57,
  BSE_MIDI_SIGNAL_CONTROL_58,
  BSE_MIDI_SIGNAL_CONTROL_59,
  BSE_MIDI_SIGNAL_CONTROL_60,
  BSE_MIDI_SIGNAL_CONTROL_61,
  BSE_MIDI_SIGNAL_CONTROL_62,
  BSE_MIDI_SIGNAL_CONTROL_63,
  /* 7bit, literal channel controls */
  BSE_MIDI_SIGNAL_CONTROL_64,		/*< nick=Control 64 Damper Pedal Switch (Sustain) >*/
  BSE_MIDI_SIGNAL_CONTROL_65,		/*< nick=Control 65 Portamento Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_66,		/*< nick=Control 66 Sustenuto Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_67,		/*< nick=Control 67 Soft Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_68,		/*< nick=Control 68 Legato Pedal Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_69,		/*< nick=Control 69 Hold Pedal Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_70,		/*< nick=Control 70 Sound Variation >*/
  BSE_MIDI_SIGNAL_CONTROL_71,		/*< nick=Control 71 Sound Timbre >*/
  BSE_MIDI_SIGNAL_CONTROL_72,		/*< nick=Control 72 Sound Release Time >*/
  BSE_MIDI_SIGNAL_CONTROL_73,		/*< nick=Control 73 Sound Attack Time >*/
  BSE_MIDI_SIGNAL_CONTROL_74,		/*< nick=Control 74 Sound Brightness >*/
  BSE_MIDI_SIGNAL_CONTROL_75,		/*< nick=Control 75 Sound Control 6 >*/
  BSE_MIDI_SIGNAL_CONTROL_76,		/*< nick=Control 76 Sound Control 7 >*/
  BSE_MIDI_SIGNAL_CONTROL_77,		/*< nick=Control 77 Sound Control 8 >*/
  BSE_MIDI_SIGNAL_CONTROL_78,		/*< nick=Control 78 Sound Control 9 >*/
  BSE_MIDI_SIGNAL_CONTROL_79,		/*< nick=Control 79 Sound Control 10 >*/
  BSE_MIDI_SIGNAL_CONTROL_80,		/*< nick=Control 80 General Purpose Switch 5 >*/
  BSE_MIDI_SIGNAL_CONTROL_81,		/*< nick=Control 81 General Purpose Switch 6 >*/
  BSE_MIDI_SIGNAL_CONTROL_82,		/*< nick=Control 82 General Purpose Switch 7 >*/
  BSE_MIDI_SIGNAL_CONTROL_83,		/*< nick=Control 83 General Purpose Switch 8 >*/
  BSE_MIDI_SIGNAL_CONTROL_84,		/*< nick=Control 84 Portamento Control (Note) >*/
  BSE_MIDI_SIGNAL_CONTROL_85,
  BSE_MIDI_SIGNAL_CONTROL_86,
  BSE_MIDI_SIGNAL_CONTROL_87,
  BSE_MIDI_SIGNAL_CONTROL_88,
  BSE_MIDI_SIGNAL_CONTROL_89,
  BSE_MIDI_SIGNAL_CONTROL_90,
  BSE_MIDI_SIGNAL_CONTROL_91,		/*< nick=Control 91 Reverb Depth >*/
  BSE_MIDI_SIGNAL_CONTROL_92,		/*< nick=Control 92 Tremolo Depth >*/
  BSE_MIDI_SIGNAL_CONTROL_93,		/*< nick=Control 93 Chorus Depth >*/
  BSE_MIDI_SIGNAL_CONTROL_94,		/*< nick=Control 93 Detune Depth >*/
  BSE_MIDI_SIGNAL_CONTROL_95,		/*< nick=Control 95 Phase Depth >*/
  BSE_MIDI_SIGNAL_CONTROL_96,		/*< nick=Control 96 Data Increment Trigger >*/
  BSE_MIDI_SIGNAL_CONTROL_97,		/*< nick=Control 97 Data Decrement Trigger >*/
  BSE_MIDI_SIGNAL_CONTROL_98,		/*< nick=Control 98 Non-Registered Parameter MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_99,		/*< nick=Control 99 Non-Registered Parameter LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_100,		/*< nick=Control 100 Registered Parameter MSB >*/
  BSE_MIDI_SIGNAL_CONTROL_101,		/*< nick=Control 101 Registered Parameter LSB >*/
  BSE_MIDI_SIGNAL_CONTROL_102,
  BSE_MIDI_SIGNAL_CONTROL_103,
  BSE_MIDI_SIGNAL_CONTROL_104,
  BSE_MIDI_SIGNAL_CONTROL_105,
  BSE_MIDI_SIGNAL_CONTROL_106,
  BSE_MIDI_SIGNAL_CONTROL_107,
  BSE_MIDI_SIGNAL_CONTROL_108,
  BSE_MIDI_SIGNAL_CONTROL_109,
  BSE_MIDI_SIGNAL_CONTROL_110,
  BSE_MIDI_SIGNAL_CONTROL_111,
  BSE_MIDI_SIGNAL_CONTROL_112,
  BSE_MIDI_SIGNAL_CONTROL_113,
  BSE_MIDI_SIGNAL_CONTROL_114,
  BSE_MIDI_SIGNAL_CONTROL_115,
  BSE_MIDI_SIGNAL_CONTROL_116,
  BSE_MIDI_SIGNAL_CONTROL_117,
  BSE_MIDI_SIGNAL_CONTROL_118,
  BSE_MIDI_SIGNAL_CONTROL_119,
  BSE_MIDI_SIGNAL_CONTROL_120,		/*< nick=Control 120 All Sound Off ITrigger >*/
  BSE_MIDI_SIGNAL_CONTROL_121,		/*< nick=Control 121 All Controllers Off ITrigger >*/
  BSE_MIDI_SIGNAL_CONTROL_122,		/*< nick=Control 122 Local Control Switch >*/
  BSE_MIDI_SIGNAL_CONTROL_123,		/*< nick=Control 123 All Notes Off ITrigger >*/
  BSE_MIDI_SIGNAL_CONTROL_124,		/*< nick=Control 124 Omni Mode Off ITrigger >*/
  BSE_MIDI_SIGNAL_CONTROL_125,		/*< nick=Control 125 Omni Mode On ITrigger >*/
  BSE_MIDI_SIGNAL_CONTROL_126,		/*< nick=Control 126 Monophonic Voices Mode >*/
  BSE_MIDI_SIGNAL_CONTROL_127		/*< nick=Control 127 Polyphonic Mode On ITrigger >*/
} BseMidiSignalType;
#endif

gfloat		bse_midi_signal_default	(BseMidiSignalType signal);
const gchar*	bse_midi_signal_name	(BseMidiSignalType signal);
const gchar*	bse_midi_signal_nick	(BseMidiSignalType signal);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_EVENT_H__ */

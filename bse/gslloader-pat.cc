/* GSL - Generic Sound Layer
 * Copyright (C) 2004 Stefan Westerfeld
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gslloader.h"
#include <stdio.h>
#include <errno.h>
#include <vector>

using std::vector;

/*
 * generic patch loading code from aRts
 */
namespace
{
  typedef unsigned char byte;
  typedef unsigned short int word;
  typedef unsigned int dword;
  typedef char sbyte;
  typedef short int sword;
  typedef int sdword;

  static int pos = 0;
  static int apos = 0;

  inline void xRead(FILE *file, int len, void *data)
  {
    //	printf("(0x%2x) - 0x%02x  ... reading %d bytes\n",apos,pos,len);
    pos += len;
    apos += len;
    if(fread(data, len, 1, file) != 1)
      fprintf(stdout, "short read\n");
  }

  inline void skip(FILE *file, int len)
  {
    //	printf("(0x%2x) - 0x%02x  ... skipping %d bytes\n",apos,pos,len);
    pos += len;
    apos += len;
    while(len > 0)
      {
	char junk;
	if(fread(&junk, 1, 1, file) != 1)
	  fprintf(stdout, "short read\n");
	len--;
      }
  }


  inline void readBytes(FILE *file, unsigned char *bytes, int len)
  {
    xRead(file, len, bytes);
  }

  inline void readString(FILE *file, char *str, int len)
  {
    xRead(file, len, str);
  }

  /* readXXX with sizeof(xxx) == 1 */
  inline void readByte(FILE *file, byte& b)
  {
    xRead(file, 1, &b);
  }

  /* readXXX with sizeof(xxx) == 2 */
  inline void readWord(FILE *file, word& w)
  {
    byte h, l;
    xRead(file, 1, &l);
    xRead(file, 1, &h);
    w = (h << 8) + l;
  }

  inline void readSWord(FILE *file, sword& sw)
  {
    word w;
    readWord(file, w);
    sw = (sword)w;
  }

  /* readXXX with sizeof(xxx) == 4 */
  inline void readDWord(FILE *file, dword& dw)
  {
    byte h, l, hh, hl;
    xRead(file, 1, &l);
    xRead(file, 1, &h);
    xRead(file, 1, &hl);
    xRead(file, 1, &hh);
    dw = (hh << 24) + (hl << 16) + (h << 8) + l;
  }

  struct PatHeader
  {
    char id[12];					/* ID='GF1PATCH110' */
    char manufacturer_id[10];		/* Manufacturer ID */
    char description[60];			/* Description of the contained Instruments
					       or copyright of manufacturer. */
    byte instruments;				/* Number of instruments in this patch */
    byte voices;					/* Number of voices for sample */
    byte channels;					/* Number of output channels
						       (1=mono,2=stereo) */
    word waveforms;					/* Number of waveforms */
    word mastervolume;				/* Master volume for all samples */
    dword size;						/* Size of the following data */
    char reserved[36];				/* reserved */

    PatHeader(FILE *file)
    {
      readString(file, id, 12);
      readString(file, manufacturer_id, 10);
      readString(file, description, 60);
      /*		skip(file, 2);*/

      readByte(file, instruments);
      readByte(file, voices);
      readByte(file, channels);

      readWord(file, waveforms);
      readWord(file, mastervolume);
      readDWord(file, size);

      readString(file, reserved, 36);
    }
  };

  struct PatInstrument
  {
    word	number;
    char	name[16];
    dword 	size;					/* Size of the whole instrument in bytes. */
    byte	layers;
    char	reserved[40];

    /* layer? */
    word	layerUnknown;
    dword	layerSize;
    byte	sampleCount;			/* number of samples in this layer (?) */
    char	layerReserved[40];

    PatInstrument(FILE *file)
    {
      readWord(file, number);
      readString(file, name, 16);
      readDWord(file, size);
      readByte(file, layers);
      readString(file, reserved, 40);

      /* layer: (?) */
      readWord(file, layerUnknown);
      readDWord(file, layerSize);
      readByte(file, sampleCount);
      readString(file, reserved, 40);
    }
  };

  enum {
    PAT_FORMAT_LOOPED = (1 << 2),
    PAT_FORMAT_LOOP_BIDI = (1 << 3),
    PAT_FORMAT_LOOP_BACKWARDS = (1 << 4)
  };

  struct PatPatch
  {
    char	filename[7];			/* Wave file name */
    byte	fractions;				/* Fractions */
    dword	wavesize;				/* Wave size.
						       Size of the wave digital data */
    dword	loopStart;
    dword	loopEnd;
    word	sampleRate;
    dword	minFreq;
    dword	maxFreq;
    dword	origFreq;
    sword	fineTune;
    byte	balance;
    byte	filterRate[6];
    byte	filterOffset[6];
    byte	tremoloSweep;
    byte	tremoloRate;
    byte	tremoloDepth;
    byte	vibratoSweep;
    byte	vibratoRate;
    byte	vibratoDepth;
    byte	waveFormat;
    sword	freqScale;
    word	freqScaleFactor;
    char	reserved[36];

    PatPatch(FILE *file)
    {
      readString(file, filename, 7);
      readByte(file, fractions);
      readDWord(file, wavesize);
      readDWord(file, loopStart);
      readDWord(file, loopEnd);
      readWord(file, sampleRate);
      readDWord(file, minFreq);
      readDWord(file, maxFreq);
      readDWord(file, origFreq);
      readSWord(file, fineTune);
      readByte(file, balance);
      readBytes(file, filterRate, 6);
      readBytes(file, filterOffset, 6);
      readByte(file, tremoloSweep);
      readByte(file, tremoloRate);
      readByte(file, tremoloDepth);
      readByte(file, vibratoSweep);
      readByte(file, vibratoRate);
      readByte(file, vibratoDepth);
      readByte(file, waveFormat);
      readSWord(file, freqScale);
      readWord(file, freqScaleFactor);
      readString(file, reserved, 36);
    }
  };
};

namespace {

/*
 * adaptation to GSL API
 */
struct FileInfo
{
  GslWaveFileInfo wfi;
  GslWaveDsc      wdsc;
  GslErrorType    error;

  PatHeader          *header;
  PatInstrument      *instrument;
  vector<PatPatch *>  patches;
  vector<long>        data_offsets;

  GslWaveLoopType loop_type (int wave_format)
  {
    /* FIXME: is backwards for the loop or for the wave? */
    if (wave_format & (PAT_FORMAT_LOOPED))
      {
	if (wave_format & (PAT_FORMAT_LOOP_BIDI))
	  {
	    if (wave_format & (PAT_FORMAT_LOOP_BACKWARDS))
	      {
		printf ("pat loader: unsupported loop type (backwards-pingpong)\n");
		return GSL_WAVE_LOOP_PINGPONG;
	      }
	    else
	      {
		return GSL_WAVE_LOOP_PINGPONG;
	      }
	  }
	else
	  {
	    if (wave_format & (PAT_FORMAT_LOOP_BACKWARDS))
	      {
		printf ("pat loader: unsupported loop type (backwards-jump)\n");
		return GSL_WAVE_LOOP_JUMP;
	      }
	    else
	      {
		return GSL_WAVE_LOOP_JUMP;
	      }
	  }
      }
    else
      {
	return GSL_WAVE_LOOP_NONE;
      }
  }

  GslWaveFormatType
  wave_format (int wave_format)
  {
    return GSL_WAVE_FORMAT_SIGNED_16; /* FIXME */
  }

  int
  bytes_per_frame (int wave_format)
  {
    return 2 * header->channels;
  }

  FileInfo (FILE *patfile, const gchar *file_name)
  {
    /* parse contents of patfile into Pat* data structurs */

    header = new PatHeader (patfile);
    if (header->channels == 0) /* fixup channels setting */
      header->channels = 1;

    instrument = new PatInstrument (patfile);

    for (int i = 0; i<instrument->sampleCount; i++)
      {
	PatPatch *patch = new PatPatch (patfile);
	data_offsets.push_back (ftell (patfile));
	skip (patfile, patch->wavesize);
	patches.push_back (patch);

	printf (" - read patch, srate = %d (%d bytes)\n", patch->sampleRate, patch->wavesize);
      }

    /* allocate and fill appropriate Gsl* data structures */

    /* fill GslWaveFileInfo */
    memset (&wfi, 0, sizeof (wfi));
    wfi.n_waves = 1;
    wfi.waves = (typeof (wfi.waves)) g_malloc0 (sizeof (wfi.waves[0]) * wfi.n_waves);
    wfi.waves[0].name = g_strdup (file_name);

    /* fill GslWaveDsc */
    memset (&wdsc, 0, sizeof (wdsc));
    wdsc.name = g_strdup (file_name);
    wdsc.n_chunks = instrument->sampleCount;
    wdsc.chunks = (typeof (wdsc.chunks)) g_malloc0 (sizeof (wdsc.chunks[0]) * wdsc.n_chunks);
    wdsc.n_channels = header->channels;

    for (int i = 0; i < instrument->sampleCount; i++)
      {
	/* fill GslWaveChunk */
	wdsc.chunks[i].mix_freq = patches[i]->sampleRate;
	wdsc.chunks[i].osc_freq = patches[i]->origFreq / 1024.0;

	int frame_size = bytes_per_frame (patches[i]->waveFormat);
	wdsc.chunks[i].loop_type = loop_type (patches[i]->waveFormat);
	wdsc.chunks[i].loop_start = patches[i]->loopStart / frame_size;
	wdsc.chunks[i].loop_end = patches[i]->loopEnd / frame_size;
	wdsc.chunks[i].loop_count = loop_type (patches[i]->waveFormat) ? 1000000 : 0; /* FIXME */
	printf ("loop settings: from %ld to %ld (total %d) type %d\n",
	                      wdsc.chunks[i].loop_start,
			      wdsc.chunks[i].loop_end,
			      patches[i]->wavesize,
			      wdsc.chunks[i].loop_type);
      }

    error = GSL_ERROR_NONE; /* FIXME: more error handling might be useful */
  }

  ~FileInfo()
  {
    delete header;
    delete instrument;

    g_free (wfi.waves);
  }
};

static GslWaveFileInfo*
pat_load_file_info (gpointer      data,
		    const gchar  *file_name,
		    GslErrorType *error_p)
{
  printf ("pat: LFI: %s\n", file_name);
  FILE *patfile = fopen (file_name, "r");
  if (!patfile)
    {
      *error_p = gsl_error_from_errno (errno, GSL_ERROR_IO);
      return NULL;
    }

  FileInfo *file_info = new FileInfo (patfile, file_name);
  *error_p = file_info->error;

  if (file_info->error)
    {
      delete file_info;
      return NULL;
    }

  return &file_info->wfi;
}

static void
pat_free_file_info (gpointer         data,
		    GslWaveFileInfo *wave_file_info)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  delete file_info;
}

static GslWaveDsc*
pat_load_wave_dsc (gpointer         data,
		   GslWaveFileInfo *wave_file_info,
		   guint            nth_wave,
		   GslErrorType    *error_p)
{
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_file_info);
  printf ("pat: LWD: %p (%d)\n", file_info, nth_wave);
  return &file_info->wdsc;
}

static void
pat_free_wave_dsc (gpointer    data,
		   GslWaveDsc *wave_dsc)
{
}

static GslDataHandle*
pat_create_chunk_handle (gpointer      data,
			 GslWaveDsc   *wave_dsc,
			 guint         nth_chunk,
			 GslErrorType *error_p)
{
  printf ("cch %d\n", nth_chunk);
  FileInfo *file_info = reinterpret_cast<FileInfo*> (wave_dsc->file_info);
  GslDataHandle *dhandle;

  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);

  PatPatch *patch = file_info->patches[nth_chunk];
  printf ("gsl_wave_handle_new %s %d %d %d %f %f %ld %d\n",
	  file_info->wfi.file_name,
	  wave_dsc->n_channels,
	  file_info->wave_format (patch->waveFormat),
	  G_LITTLE_ENDIAN,
	  wave_dsc->chunks[nth_chunk].mix_freq,
	  wave_dsc->chunks[nth_chunk].osc_freq,
	  file_info->data_offsets[nth_chunk],
	  patch->wavesize / file_info->bytes_per_frame (patch->waveFormat));

  dhandle = gsl_wave_handle_new (file_info->wfi.file_name,
	                         wave_dsc->n_channels,
				 file_info->wave_format (file_info->patches[nth_chunk]->waveFormat),
				 G_LITTLE_ENDIAN,
				 wave_dsc->chunks[nth_chunk].mix_freq,
				 wave_dsc->chunks[nth_chunk].osc_freq,
				 file_info->data_offsets[nth_chunk],
				 patch->wavesize / file_info->bytes_per_frame (patch->waveFormat));
  return dhandle;
}

} // namespace

extern "C" void
_gsl_init_loader_pat()
{
  static const gchar *file_exts[] = { "pat", NULL, };
  static const gchar *mime_types[] = { "audio/x-pat", NULL, }; // FIXME: correct?
  /*
   * file uses:
   * 0   string      GF1PATCH110\0ID#000002\0    GUS patch
   * 0   string      GF1PATCH100\0ID#000002\0    Old GUS patch
   *
   * we use GF1PATCH1 which is a superset (but maybe more than desired?)
   */
  static const gchar *magics[] = {
    (
     "0  string  GF1PATCH1"
     ),
    NULL,
  };
  static GslLoader loader = {
    "GUS Patch",
    file_exts,
    mime_types,
    static_cast<GslLoaderFlags> (0),	/* flags */
    magics,
    0,  /* priority */
    NULL,
    pat_load_file_info,
    pat_free_file_info,
    pat_load_wave_dsc,
    pat_free_wave_dsc,
    pat_create_chunk_handle,
  };
  static gboolean initialized = FALSE;
  
  g_assert (initialized == FALSE);
  initialized = TRUE;
  
  gsl_loader_register (&loader);
}

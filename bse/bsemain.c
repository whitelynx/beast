/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#include	"gslconfig.h"
#include	"bsemain.h"

#include	"bseserver.h"
#include	"bseplugin.h"
#include	"bsecategories.h"
#include	"gslcommon.h"
#include	"gslengine.h"
#include	<string.h>
#include	<stdlib.h>
#include	<sys/time.h>


/* --- variables --- */
static gboolean bse_is_initialized = FALSE;
static SfiMutex sequencer_mutex;
BseDebugFlags   bse_debug_flags = 0;
gboolean	bse_developer_extensions = FALSE;


/* --- functions --- */
gboolean
bse_initialized (void)
{
  return bse_is_initialized;
}

static void
bse_parse_args (gint    *argc_p,
		gchar ***argv_p)
{
  extern GFlagsValue *bse_debug_key_flag_values;	/* bseenums.c feature */
  extern guint        bse_debug_key_n_flag_values;	/* bseenums.c feature */
  GDebugKey *debug_keys;
  guint n_debug_keys;
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;
  
  debug_keys = g_new (GDebugKey, bse_debug_key_n_flag_values);
  for (i = 0; i < bse_debug_key_n_flag_values && bse_debug_key_flag_values[i].value_nick; i++)
    {
      debug_keys[i].key = bse_debug_key_flag_values[i].value_nick;
      debug_keys[i].value = bse_debug_key_flag_values[i].value;
    }
  n_debug_keys = i;
  
  envar = getenv ("BSE_DEBUG");
  if (envar)
    {
      guint op_lvl;
      
      bse_debug_flags |= g_parse_debug_string (envar, debug_keys, n_debug_keys);
      op_lvl = g_parse_debug_string (envar, gsl_debug_keys, gsl_n_debug_keys);
      gsl_debug_enable (op_lvl);
    }
  envar = getenv ("BSE_NO_DEBUG");
  if (envar)
    {
      guint op_lvl;
      
      bse_debug_flags &= ~g_parse_debug_string (envar, debug_keys, n_debug_keys);
      op_lvl = g_parse_debug_string (envar, gsl_debug_keys, gsl_n_debug_keys);
      gsl_debug_disable (op_lvl);
    }
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;
	  
	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);
	  
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-debug", argv[i]) == 0 ||
	       strncmp ("--bse-debug=", argv[i], 12) == 0)
	{
	  gchar *equal = argv[i] + 11;
	  guint op_lvl = 0;
	  
	  if (*equal == '=')
	    {
	      bse_debug_flags |= g_parse_debug_string (equal + 1, debug_keys, n_debug_keys);
	      op_lvl = g_parse_debug_string (equal + 1, gsl_debug_keys, gsl_n_debug_keys);
	    }
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags |= g_parse_debug_string (argv[i + 1],
						       debug_keys,
						       n_debug_keys);
	      op_lvl = g_parse_debug_string (argv[i + 1], gsl_debug_keys, gsl_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  gsl_debug_enable (op_lvl);
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-no-debug", argv[i]) == 0 ||
	       strncmp ("--bse-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;
	  guint op_lvl = 0;
	  
	  if (*equal == '=')
	    {
	      bse_debug_flags &= ~g_parse_debug_string (equal + 1, debug_keys, n_debug_keys);
	      op_lvl = g_parse_debug_string (equal + 1, gsl_debug_keys, gsl_n_debug_keys);
	    }
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags &= ~g_parse_debug_string (argv[i + 1],
							debug_keys,
							n_debug_keys);
	      op_lvl = g_parse_debug_string (argv[i + 1], gsl_debug_keys, gsl_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  gsl_debug_disable (op_lvl);
	  argv[i] = NULL;
	}
    }
  g_free (debug_keys);
  
  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;
}

static void
dummy_nop (gpointer data)
{
}

static BseLockFuncs        bse_lock_funcs = { NULL, dummy_nop, dummy_nop };

void
bse_init (int	             *argc_p,
	  char	           ***argv_p,
	  const BseLockFuncs *lock_funcs)
{
  struct timeval tv;
  gchar *dir;
  
  g_return_if_fail (bse_is_initialized == FALSE);
  
  bse_is_initialized = TRUE;
  if (lock_funcs)
    bse_lock_funcs = *lock_funcs;
  
  /* initialize submodules */
  sfi_init ();
  
  sfi_mutex_init (&sequencer_mutex);

  g_assert (BSE_BYTE_ORDER == BSE_LITTLE_ENDIAN || BSE_BYTE_ORDER == BSE_BIG_ENDIAN);
  
  /* initialize random numbers */
  gettimeofday (&tv, NULL);
  srand (tv.tv_sec ^ tv.tv_usec);
  
  if (argc_p && argv_p)
    {
      g_set_prgname (**argv_p);
      bse_parse_args (argc_p, argv_p);
    }
  
  bse_globals_init ();
  _bse_init_categories ();
  bse_type_init ();

  dir = g_get_current_dir ();
  sfi_com_set_spawn_dir (dir);
  g_free (dir);
  
  {
    static const GslConfigValue gslconfig[] = {
      { "wave_chunk_padding",		BSE_MAX_BLOCK_PADDING, },
      { "wave_chunk_big_pad",		256, },
      { "dcache_block_size",		4000, },
      { "dcache_cache_memory",		10 * 1024 * 1024, },
      { "midi_kammer_note",		BSE_KAMMER_NOTE, },
      { "kammer_freq",			BSE_KAMMER_FREQUENCY_f, },
    };
    
    gsl_init (gslconfig);
  }
  
  _bse_midi_init ();
  
  bse_plugins_init ();
  
  /* make sure the server is alive */
  bse_server_get ();
}

void
bse_main_global_lock (void)
{
  bse_lock_funcs.lock (bse_lock_funcs.lock_data);
}

void
bse_main_global_unlock (void)
{
  bse_lock_funcs.unlock (bse_lock_funcs.lock_data);
}

void
bse_main_sequencer_lock (void)
{
  GSL_SYNC_LOCK (&sequencer_mutex);
}

void
bse_main_sequencer_unlock (void)
{
  GSL_SYNC_UNLOCK (&sequencer_mutex);
}

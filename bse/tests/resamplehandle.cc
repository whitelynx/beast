/* BSE Resampling Datahandles Test
 * Copyright (C) 2001-2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
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
#include <bse/bsemathsignal.h>
#include <bse/bsemain.h>
// #define TEST_VERBOSE
#include <sfi/sfitests.h>
#include <bse/gsldatautils.h>
#include <bse/bseblockutils.hh>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;
using std::max;
using std::min;
using std::map;

static void
read_through (GslDataHandle *handle)
{
  int64 n_values = gsl_data_handle_n_values (handle);
  int64 offset = 0;

  while (offset < n_values)
    {
      gfloat values[1024];
      int64 values_read = gsl_data_handle_read (handle, offset, 1024, values);
      g_assert (values_read > 0);
      offset += values_read;
    }

  g_assert (offset == n_values);
}

static double
check (const char           *up_down,
       const char           *channels,
       uint                  bits,
       const char           *cpu_type,
       const vector<float>  &input,
       const vector<double> &expected,
       int                   n_channels,
       BseResampler2Mode     resampler_mode,
       int                   precision_bits,
       double                max_db)
{
  char *samplestr = g_strdup_printf ("ResampleHandle-%s%02d%s", up_down, bits, channels);
  char *streamstr = g_strdup_printf ("CPU Resampling %s%02d%s", up_down, bits, channels);
  TSTART ("%s (%s)", samplestr, cpu_type);

  TASSERT (input.size() % n_channels == 0);
  
  GslDataHandle *ihandle = gsl_data_handle_new_mem (n_channels, 32, 44100, 440, input.size(), &input[0], NULL);
  GslDataHandle *rhandle;
  if (resampler_mode == BSE_RESAMPLER2_MODE_UPSAMPLE)
    {
      TASSERT (input.size() * 2 == expected.size());
      rhandle = bse_data_handle_new_upsample2 (ihandle, precision_bits);
    }
  else
    {
      TASSERT (input.size() == expected.size() * 2);
      rhandle = bse_data_handle_new_downsample2 (ihandle, precision_bits); 
    }
  gsl_data_handle_unref (ihandle);

  BseErrorType error = gsl_data_handle_open (rhandle);
  TASSERT (error == 0);

  double worst_diff, worst_diff_db;

  /* Read through the datahandle linearily _twice_, and compare expected output
   * with actual output, to determine whether the actual output is correct.
   *
   * This checks four things:
   * - the datahandle introduces no delay (or shifts the signal in the other
   *   direction; a negative delay)
   * - the datahandle resamples the signal correctly
   * - the initial state of the datahandle is correct (that is, when you first
   *   read from it, it starts correctly reading at the beginning)
   * - the seek-to-position zero after reading the datahandle works correctly,
   *   that is, you get the same output when reading the datahandle a second
   *   time
   */
  for (int repeat = 1; repeat <= 2; repeat++)
    {
      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      worst_diff = 0.0;
      for (int64 i = 0; i < rhandle->setup.n_values; i++)
	{
	  double resampled = gsl_data_handle_peek_value (rhandle, i, &peek_buffer);
	  worst_diff = max (fabs (resampled - expected[i]), worst_diff);
	}
      worst_diff_db = bse_db_from_factor (worst_diff, -200);
      TPRINT ("linear(%dst read) read worst_diff = %f (%f dB)\n", repeat, worst_diff, worst_diff_db);
      TASSERT (worst_diff_db < max_db);
    }

  /* test seeking */
  worst_diff = 0.0;
  const uint count = sfi_init_settings().test_slow ? 300 : 100;
  for (uint j = 0; j < count; j++)
    {
      int64 start = rand() % rhandle->setup.n_values;
      int64 len = rand() % 1024;

      GslDataPeekBuffer peek_buffer = { +1 /* incremental direction */, 0, };
      for (int64 i = start; i < std::min (start + len, rhandle->setup.n_values); i++)
	{
	  double resampled = gsl_data_handle_peek_value (rhandle, i, &peek_buffer);
	  worst_diff = max (fabs (resampled - expected[i]), worst_diff);
	}
    }
  worst_diff_db = bse_db_from_factor (worst_diff, -200);
  TPRINT ("seek worst_diff = %f (%f dB)\n", worst_diff, worst_diff_db);
  TASSERT (worst_diff_db < max_db);

  TDONE();

  /* test speed */
  double samples_per_second = 0;
  if (sfi_init_settings().test_perf)
    {
      const guint RUNS = 10;
      GTimer *timer = g_timer_new();
      const guint dups = TEST_CALIBRATION (50.0, read_through (rhandle));
      
      double m = 9e300;
      for (guint i = 0; i < RUNS; i++)
        {
          g_timer_start (timer);
          for (guint j = 0; j < dups; j++)
            read_through (rhandle);
          g_timer_stop (timer);
          double e = g_timer_elapsed (timer, NULL);
          if (e < m)
            m = e;
        }
      samples_per_second = input.size() / (m / dups);
      treport_maximized (samplestr, samples_per_second, TUNIT (SAMPLE, SECOND));
      treport_maximized (streamstr, samples_per_second / 44100.0, TUNIT_STREAM);
      //TPRINT ("  samples / second = %f\n", samples_per_second);
      //TPRINT ("  which means the resampler can process %.2f 44100 Hz streams simultaneusly\n", samples_per_second / 44100.0);
      //TPRINT ("  or one 44100 Hz stream takes %f %% CPU usage\n", 100.0 / (samples_per_second / 44100.0));
    }

  gsl_data_handle_close (rhandle);
  gsl_data_handle_unref (rhandle);

  g_free (samplestr);
  g_free (streamstr);

  return samples_per_second / 44100.0;
}

template<typename Sample> static void
generate_test_signal (vector<Sample> &signal,
		      const size_t    signal_length,
		      const double    sample_rate,
                      const double    frequency1,
		      const double    frequency2 = -1)
{
  static map<size_t, vector<float> > window_cache;
  vector<float>& cached_window = window_cache[signal_length];
  if (cached_window.empty())
    {
      cached_window.resize (signal_length);

      for (size_t i = 0; i < signal_length; i++)
        {
          double wpos = (i * 2 - double (signal_length)) / signal_length;
          cached_window[i] = bse_window_blackman (wpos);
        }
    }

  string signal_cache_key = Birnet::string_printf ("%zd/%.1f/%.1f/%.1f", signal_length, sample_rate, frequency1, frequency2);
  static map<string, vector<Sample> > signal_cache;
  vector<Sample>& cached_signal = signal_cache[signal_cache_key];

  if (cached_signal.empty())
    {
      for (size_t i = 0; i < signal_length; i++)
        {
          double phase1 = i * 2 * M_PI * frequency1 / sample_rate;
          cached_signal.push_back (sin (phase1) * cached_window[i]);

          if (frequency2 > 0)   /* stereo */
            {
              double phase2 = i * 2 * M_PI * frequency2 / sample_rate;
              cached_signal.push_back (sin (phase2) * cached_window[i]);
            }
        }
    }
  signal = cached_signal;
}

static void
run_tests (const char *run_type)
{
  struct TestParameters {
    int bits;
    double mono_upsample_db;
    double stereo_upsample_db;
    double mono_downsample_db;
    double stereo_downsample_db;
  } params[] =
  {
    {  8,  -48,  -48, -48,   -48 },
    { 12,  -72,  -72, -72,   -72 },
    { 16,  -98,  -95, -96,   -96 },
    { 20, -120, -117, -120, -120 },
    { 24, -125, -125, -134, -131 },
    { 0, 0, 0 }
  };

  for (int p = 0; params[p].bits; p++)
    {
      const int LEN = 44100 / 2;  /* 500ms test signal */
      vector<float> input;
      vector<double> expected;

      // mono upsampling test
      if (!sfi_init_settings().test_quick)
        {
          generate_test_signal (input, LEN, 44100, 440);
          generate_test_signal (expected, LEN * 2, 88200, 440);
          check ("Up", "M", params[p].bits, run_type,
                 input, expected, 1, BSE_RESAMPLER2_MODE_UPSAMPLE,
                 params[p].bits, params[p].mono_upsample_db);
          // g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
        }
      
      // stereo upsampling test
      if (1)
        {
          generate_test_signal (input, LEN, 44100, 440, 1000);
          generate_test_signal (expected, LEN * 2, 88200, 440, 1000);
          check ("Up", "S", params[p].bits, run_type,
                 input, expected, 2, BSE_RESAMPLER2_MODE_UPSAMPLE,
                 params[p].bits, params[p].stereo_upsample_db);
          // g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
        }
      
      // mono downsampling test
      if (!sfi_init_settings().test_quick)
        {
          generate_test_signal (input, LEN, 44100, 440);
          generate_test_signal (expected, LEN / 2, 22050, 440);
          check ("Dn", "M", params[p].bits, run_type,
                 input, expected, 1, BSE_RESAMPLER2_MODE_DOWNSAMPLE,
                 params[p].bits, params[p].mono_downsample_db);
          // g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
        }
      
      // stereo downsampling test
      if (1)
        {
          generate_test_signal (input, LEN, 44100, 440, 1000);
          generate_test_signal (expected, LEN / 2, 22050, 440, 1000);
          check ("Dn", "S", params[p].bits, run_type,
                 input, expected, 2, BSE_RESAMPLER2_MODE_DOWNSAMPLE,
                 params[p].bits, params[p].stereo_downsample_db);
          // g_printerr ("    ===> speed is equivalent to %.2f simultaneous 44100 Hz streams\n", streams);
        }
    }
}

static void
test_c_api (const char *run_type)
{
  TSTART ("Resampler C API (%s)", run_type);
  BseResampler2 *resampler = bse_resampler2_create (BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_96DB);
  const int INPUT_SIZE = 1024, OUTPUT_SIZE = 2048;
  float in[INPUT_SIZE];
  float out[OUTPUT_SIZE];
  double error = 0;
  int i;

  for (i = 0; i < INPUT_SIZE; i++)
    in[i] = sin (i * 440 * 2 * M_PI / 44100) * bse_window_blackman ((double) (i * 2 - INPUT_SIZE) / INPUT_SIZE);

  bse_resampler2_process_block (resampler, in, INPUT_SIZE, out);

  int delay = bse_resampler2_delay (resampler);
  for (i = 0; i < 2048; i++)
    {
      double expected = sin ((i - delay) * 220 * 2 * M_PI / 44100)
	              * bse_window_blackman ((double) ((i - delay) * 2 - OUTPUT_SIZE) / OUTPUT_SIZE);
      error = MAX (error, fabs (out[i] - expected));
    }

  double error_db = bse_db_from_factor (error, -200);

  bse_resampler2_destroy (resampler);

  TPRINT ("Test C API delta: %f\n", error_db);
  TASSERT (error_db < -95);
  TDONE();
}

static void
test_delay_compensation (const char *run_type)
{
  struct TestParameters {
    double error_db;
    BseResampler2Mode mode;
    BseResampler2Precision precision;
  } params[] =
  {
    { 200, BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_48DB },
    { 200, BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_72DB },
    { 200, BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_96DB },
    { 200, BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_120DB },
    { 200, BSE_RESAMPLER2_MODE_UPSAMPLE, BSE_RESAMPLER2_PREC_144DB },
    { 48,  BSE_RESAMPLER2_MODE_DOWNSAMPLE, BSE_RESAMPLER2_PREC_48DB },
    { 67,  BSE_RESAMPLER2_MODE_DOWNSAMPLE, BSE_RESAMPLER2_PREC_72DB },
    { 96,  BSE_RESAMPLER2_MODE_DOWNSAMPLE, BSE_RESAMPLER2_PREC_96DB },
    { 120, BSE_RESAMPLER2_MODE_DOWNSAMPLE, BSE_RESAMPLER2_PREC_120DB },
    { 134, BSE_RESAMPLER2_MODE_DOWNSAMPLE, BSE_RESAMPLER2_PREC_144DB },
    { -1, }
  };

  using Bse::Resampler::Resampler2;
  TSTART ("Resampler Delay Compensation (%s)", run_type);

  for (guint p = 0; params[p].error_db > 0; p++)
    {
      /* setup test signal and empty output signal space */
      const int INPUT_SIZE = 44100 * 4, OUTPUT_SIZE = INPUT_SIZE * 2;

      vector<float> in (INPUT_SIZE);
      vector<float> out (OUTPUT_SIZE);

      generate_test_signal (in, INPUT_SIZE, 44100, 440);

      /* up/downsample test signal */
      Resampler2 *resampler = Resampler2::create (params[p].mode,
                                                  params[p].precision);
      resampler->process_block (&in[0], INPUT_SIZE, &out[0]);

      /* setup increments for comparision loop */
      size_t iinc = 1, jinc = 1;
      if (params[p].mode == BSE_RESAMPLER2_MODE_UPSAMPLE)
	jinc = 2;
      else
	iinc = 2;

      /* compensate resampler delay by incrementing comparision start offset */
      double delay = resampler->delay();
      size_t i = 0, j = (int) round (delay * 2);
      if (j % 2)
	{
	  /* implement half a output sample delay (for downsampling only) */
	  g_assert (params[p].mode == BSE_RESAMPLER2_MODE_DOWNSAMPLE);
	  i++;
	  j += 2;
	}
      j /= 2;

      /* actually compare source and resampled signal (one with a stepping of 2) */
      double error = 0;
      while (i < in.size() && j < out.size())
	{
	  error = MAX (error, fabs (out[j] - in[i]));
	  i += iinc; j += jinc;
	}

      delete resampler;

      /* check error against bound */
      double error_db = bse_db_from_factor (error, -250);
      TPRINT ("Resampler Delay Compensation delta: %f\n", error_db);
      TASSERT (error_db < -params[p].error_db);
    }
  TDONE();
}

static void
test_state_length (const char *run_type)
{
  TSTART ("Resampler State Length Info (%s)", run_type);

  //-----------------------------------------------------------------------------------
  // usampling
  //-----------------------------------------------------------------------------------
  {
    const guint period_size = 107;

    /* fill input with 2 periods of a sine wave, so that while at the start and
     * at the end clicks occur (because the unwindowed signal is assumed to 0 by
     * the resamplehandle), in the middle 1 period can be found that is clickless
     */
    vector<float> input (period_size * 2);
    for (size_t i = 0; i < input.size(); i++)
      input[i] = sin (i * 2 * M_PI / period_size);

    const guint precision_bits = 16;
    GslDataHandle *ihandle = gsl_data_handle_new_mem (1, 32, 44100, 440, input.size(), &input[0], NULL);
    GslDataHandle *rhandle = bse_data_handle_new_upsample2 (ihandle, precision_bits);
    BseErrorType open_error = gsl_data_handle_open (rhandle);
    TASSERT (open_error == 0);
    TASSERT (gsl_data_handle_get_state_length (ihandle) == 0);

    // determine how much of the end of the signal is "unusable" due to the resampler state:
    const int64 state_length = gsl_data_handle_get_state_length (rhandle);

    /* read resampled signal in the range unaffected by the resampler state (that
     * is: not at the directly at the beginning, and not directly at the end)
     */
    vector<float> output (input.size() * 3);
    for (size_t values_done = 0; values_done < output.size(); values_done++)
      {
	/* NOTE: this is an inlined implementation of a loop, which you normally would
	 * implement with a loop handle, and it is inefficient because we read the
	 * samples one-by-one -> usually: don't use such code, always read in blocks */
	int64 read_pos = (values_done + state_length) % (period_size * 2) + (period_size * 2 - state_length);
	TCHECK (read_pos >= state_length);   /* check that input signal was long enough to be for this test */
	int64 values_read = gsl_data_handle_read (rhandle, read_pos, 1, &output[values_done]);
	TCHECK (values_read == 1);
      }
    double error = 0;
    for (size_t i = 0; i < output.size(); i++)
      {
	double expected = sin (i * 2 * M_PI / (period_size * 2));
	error = MAX (error, fabs (output[i] - expected));
      }
    double error_db = bse_db_from_factor (error, -200);
    TASSERT (error_db < -97);
  }

  //-----------------------------------------------------------------------------------
  // downsampling
  //-----------------------------------------------------------------------------------

  {
    const guint period_size = 190;

    /* fill input with 2 periods of a sine wave, so that while at the start and
     * at the end clicks occur (because the unwindowed signal is assumed to 0 by
     * the resamplehandle), in the middle 1 period can be found that is clickless
     */
    vector<float> input (period_size * 2);
    for (size_t i = 0; i < input.size(); i++)
      input[i] = sin (i * 2 * M_PI / period_size);

    const guint precision_bits = 16;
    GslDataHandle *ihandle = gsl_data_handle_new_mem (1, 32, 44100, 440, input.size(), &input[0], NULL);
    GslDataHandle *rhandle = bse_data_handle_new_downsample2 (ihandle, precision_bits);
    BseErrorType open_error = gsl_data_handle_open (rhandle);
    TASSERT (open_error == 0);
    TASSERT (gsl_data_handle_get_state_length (ihandle) == 0);

    // determine how much of the end of the signal is "unusable" due to the resampler state:
    const int64 state_length = gsl_data_handle_get_state_length (rhandle);

    /* read resampled signal in the range unaffected by the resampler state (that
     * is: not at the directly at the beginning, and not directly at the end)
     */
    vector<float> output (input.size() * 3 / 2);
    for (size_t values_done = 0; values_done < output.size(); values_done++)
      {
	/* NOTE: this is an inlined implementation of a loop, which you normally would
	 * implement with a loop handle, and it is inefficient because we read the
	 * samples one-by-one -> usually: don't use such code, always read in blocks */
	int64 read_pos = (values_done + state_length) % (period_size / 2) + (period_size / 2 - state_length);
	TCHECK (read_pos >= state_length);   /* check that input signal was long enough to be for this test */
	int64 values_read = gsl_data_handle_read (rhandle, read_pos, 1, &output[values_done]);
	TCHECK (values_read == 1);
      }
    double error = 0;
    for (size_t i = 0; i < output.size(); i++)
      {
	double expected = sin (i * 2 * M_PI / (period_size / 2));
	error = MAX (error, fabs (output[i] - expected));
      }
    double error_db = bse_db_from_factor (error, -200);
    TASSERT (error_db < -105);
  }
  TDONE();
}


int
main (int   argc,
      char *argv[])
{
  sfi_init_test (&argc, &argv, NULL);
  { /* bse_init_test() usually does this for us */
    SfiCPUInfo ci = sfi_cpu_info();
    char *cname = g_strdup_printf ("%s+%s", ci.machine, bse_block_impl_name());
    treport_cpu_name (cname);
    g_free (cname);
  }
  
  test_c_api ("FPU");
  test_delay_compensation ("FPU");
  test_state_length ("FPU");
  run_tests ("FPU");

  /* load plugins */
  SfiInitValue config[] = {
    { "load-core-plugins", "1" },
    { NULL },
  };
  bse_init_test (&argc, &argv, config);
  /* check for possible specialization */
  if (Bse::Block::default_singleton() == Bse::Block::current_singleton())
    return 0;   /* nothing changed */

  test_c_api ("SSE");
  test_delay_compensation ("SSE");
  test_state_length ("SSE");
  run_tests ("SSE");

  return 0;
}

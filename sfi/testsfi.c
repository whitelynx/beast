/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#include <sfi.h>
#include <unistd.h>

/* provide IDL type initializers */
#define sfidl_pspec_Real(name, nick, blurb, dflt, min, max, step, hints)  \
  sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints)
#define sfidl_pspec_BoxedRec(name, nick, blurb, hints, fields)            \
  sfi_pspec_rec (name, nick, blurb, fields, hints)
#define sfidl_pspec_Choice(name, nick, blurb, default_value, hints, choices) \
  sfi_pspec_choice (name, nick, blurb, default_value, choices, hints)

/* FIXME: small hackery */
#define sfidl_pspec_Rec(name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)
#define sfidl_pspec_PSpec(name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)

#include "testidl.h"

#define	MSG(what)	do g_print ("%s [", what); while (0)
#define	TICK()		do g_print ("-"); while (0)
#define	XTICK()		do g_print ("X"); while (0)
#define	DONE()		do g_print ("]\n"); while (0)
#define	ASSERT(code)	do { if (code) TICK (); else g_error ("failed to assert: %s", G_STRINGIFY (code)); } while (0)

static void
test_misc (void)
{
  MSG ("Misc:");
  ASSERT (0 == 0);
  DONE ();
}

static void
test_com_ports (void)
{
  gint afds[2], pipe_error;
  SfiComPort *port1, *port2;
  GValue *value, *rvalue;
  MSG ("Communication Ports:");
  pipe_error = pipe (afds);
  ASSERT (pipe_error == 0);
  port1 = sfi_com_port_from_pipe ("portA", -1, afds[1]);
  ASSERT (port1->connected == TRUE);
  port2 = sfi_com_port_from_pipe ("portB", afds[0], -1);
  ASSERT (port2->connected == TRUE);
  /* transport a value */
  {	/* create complex value */
    GParamSpec *pspec = sfi_pspec_log_scale ("name", "Nick", "The Blurb", 440, 110, 1760, 0.03, 440, 2, 2, SFI_PARAM_GUI);
    SfiRec *rec = sfi_pspec_to_rec (pspec);
    g_param_spec_ref (pspec);
    g_param_spec_sink (pspec);
    g_param_spec_unref (pspec);
    value = sfi_value_rec (rec);
    sfi_rec_unref (rec);
  }
  sfi_com_port_send (port1, value);
  rvalue = sfi_com_port_recv (port2);
  ASSERT (rvalue != NULL);
  {	/* assert equality of values */
    GString *s1 = g_string_new (NULL), *s2 = g_string_new (NULL);
    sfi_value_store_typed (value, s1);
    sfi_value_store_typed (rvalue, s2);
    ASSERT (strcmp (s1->str, s2->str) == 0);
    g_string_free (s1, TRUE);
    g_string_free (s2, TRUE);
  }
  sfi_value_free (value);
  sfi_value_free (rvalue);
  sfi_com_port_close_remote (port1, TRUE);
  pipe_error = close (afds[1]);
  ASSERT (pipe_error == -1);
  sfi_com_port_close_remote (port2, TRUE);
  pipe_error = close (afds[0]);
  ASSERT (pipe_error == -1);
  sfi_com_port_unref (port1);
  sfi_com_port_unref (port2);
  DONE ();
}

static void
test_thread (gpointer data)
{
  guint *tdata = data;
  sfi_thread_sleep (-1);
  *tdata += 1;
  while (!sfi_thread_aborted ())
    sfi_thread_sleep (-1);
  XTICK ();
}

static void
test_threads (void)
{
  guint thread_data = 0;
  SfiThread *thread;
  MSG ("Threading:");
  thread = sfi_thread_run (NULL, test_thread, &thread_data);
  ASSERT (thread != NULL);
  ASSERT (thread_data == 0);
  sfi_thread_wakeup (thread);
  sfi_thread_abort (thread);
  ASSERT (thread_data > 0);
  DONE ();
}

#define SCANNER_ASSERT(scanner, printout, token, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  ASSERT (g_scanner_get_next_token (scanner) == token); \
  if (printout) g_print ("{scanner64value:%llu}", scanner->value.v_int64); \
  ASSERT (scanner->value.v_int64 == svalue); \
  ASSERT (g_scanner_get_next_token (scanner) == '#'); \
}

static void
test_scanner64 (void)
{
  GScanner *scanner = g_scanner_new (sfi_storage_scanner_config);
  MSG ("64Bit Scanner:");
  scanner->config->numbers_2_int = FALSE;
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b10000000000000000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b11111111111111111111111111111111 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b1111111111111111111111111111111111111111111111111111111111111111 #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 0200000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 037777777777 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 01777777777777777777777 #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0x0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0x10000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0xffffffff #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0xffffffffffffffff #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 65536 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 4294967295 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 18446744073709551615 #", 18446744073709551615U);
  g_scanner_destroy (scanner);
  DONE ();
}

typedef enum /*< skip >*/
{
  SERIAL_TEST_TYPED = 1,
  SERIAL_TEST_PARAM,
  SERIAL_TEST_PSPEC
} SerialTest;

static SerialTest serial_test_type = 0;

static void
serial_pspec_check (GParamSpec *pspec,
		    GScanner   *scanner)
{
  GValue *value = sfi_value_pspec (pspec), rvalue = { 0, };
  GString *s1 = g_string_new (NULL);
  GString *s2 = g_string_new (NULL);
  GTokenType token;
  sfi_value_store_typed (value, s1);
  g_scanner_input_text (scanner, s1->str, s1->len);
  token = sfi_value_parse_typed (&rvalue, scanner);
  if (token != G_TOKEN_NONE)
    {
      g_print ("{while parsing pspec \"%s\":\n\t%s\n", pspec->name, s1->str);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
			     g_strdup_printf ("failed to serialize pspec \"%s\"", pspec->name), TRUE);
    }
  ASSERT (token == G_TOKEN_NONE);
  sfi_value_store_typed (&rvalue, s2);
  if (strcmp (s1->str, s2->str))
    g_print ("{while comparing pspecs \"%s\":\n\t%s\n\t%s\n", pspec->name, s1->str, s2->str);
  ASSERT (strcmp (s1->str, s2->str) == 0);
  g_value_unset (&rvalue);
  sfi_value_free (value);
  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
}

static void
serialize_cmp (GValue     *value,
	       GParamSpec *pspec)
{
  GScanner *scanner = g_scanner_new (sfi_storage_scanner_config);
  GString *gstring = g_string_new (NULL);
  GValue rvalue = { 0, };
  GTokenType token;
  gint cmp;
  if (serial_test_type == SERIAL_TEST_PSPEC)
    serial_pspec_check (pspec, scanner);
  else
    {
      if (serial_test_type == SERIAL_TEST_TYPED)
	sfi_value_store_typed (value, gstring);
      else /* SERIAL_TEST_PARAM */
	sfi_value_store_param (value, gstring, pspec, 2);
      g_scanner_input_text (scanner, gstring->str, gstring->len);
      if (serial_test_type == SERIAL_TEST_TYPED)
	token = sfi_value_parse_typed (&rvalue, scanner);
      else /* SERIAL_TEST_PARAM */
	{
	  if (g_scanner_get_next_token (scanner) == '(')
	    if (g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER &&
		strcmp (scanner->value.v_identifier, pspec->name) == 0)
	      token = sfi_value_parse_param_rest (&rvalue, scanner, pspec);
	    else
	      token = G_TOKEN_IDENTIFIER;
	  else
	    token = '(';
	}
      if (0)
	g_print ("{parsing:%s}", gstring->str);
      if (token != G_TOKEN_NONE)
	{
	  g_print ("{while parsing \"%s\":\n\t%s\n", pspec->name, gstring->str);
	  g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
				 g_strdup_printf ("failed to serialize \"%s\"", pspec->name), TRUE);
	}
      ASSERT (token == G_TOKEN_NONE);
      cmp = g_param_values_cmp (pspec, value, &rvalue);
      if (cmp)
	{
	  g_print ("{after parsing:\n\t%s\n", gstring->str);
	  g_print ("while comparing:\n\t%s\nwith:\n\t%s\nresult:\n\t%d\n",
		   g_strdup_value_contents (value),
		   g_strdup_value_contents (&rvalue),
		   cmp);
	  if (0)
	    {
	      G_BREAKPOINT ();
	      g_value_unset (&rvalue);
	      g_scanner_input_text (scanner, gstring->str, gstring->len);
	      token = sfi_value_parse_typed (&rvalue, scanner);
	    }
	}
      ASSERT (cmp == 0);
      /* generate testoutput: g_print ("OK=================(%s)=================:\n%s\n", pspec->name, gstring->str); */
    }
  g_scanner_destroy (scanner);
  g_string_free (gstring, TRUE);
  if (G_VALUE_TYPE (&rvalue))
    g_value_unset (&rvalue);
  sfi_value_free (value);
  sfi_pspec_sink (pspec);
}

static void
test_typed_serialization (SerialTest test_type)
{
  static const SfiChoiceValue test_choices[] = {
    { "ozo-foo", "Ozo-foo blurb", },
    { "emptyblurb", "", },
    { "noblurb", NULL, },
  };
  static const SfiChoiceValues choice_values = { G_N_ELEMENTS (test_choices), test_choices };
  SfiRecFields rec_fields = { 0, NULL, };
  GParamSpec *pspec_homo_seq;
  SfiFBlock *fblock;
  SfiBBlock *bblock;
  SfiSeq *seq;
  SfiRec *rec;
  GValue *val;
  gchar str256[257];
  guint i;
  serial_test_type = test_type;
  switch (serial_test_type)
    {
    case SERIAL_TEST_TYPED:	MSG ("Typed Serialization:");	break;
    case SERIAL_TEST_PARAM:	MSG ("Param Serialization:");	break;
    case SERIAL_TEST_PSPEC:	MSG ("Pspec Serialization:");	break;
    }
  serialize_cmp (sfi_value_bool (FALSE),
		 sfi_pspec_bool ("bool-false", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bool (TRUE),
		 sfi_pspec_bool ("bool-true", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MAXINT),
		 sfi_pspec_int ("int-max", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MININT),
		 sfi_pspec_int ("int-min", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MAXNUM),
		 sfi_pspec_num ("num-max", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MINNUM),
		 sfi_pspec_num ("num-min", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MAXREAL),
		 sfi_pspec_real ("real-max", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINREAL),
		 sfi_pspec_real ("real-min", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MINREAL),
		 sfi_pspec_real ("real-min-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MAXREAL),
		 sfi_pspec_real ("real-max-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MININT),
		 sfi_pspec_real ("real-minint", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINNUM),
		 sfi_pspec_real ("real-minnum", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("vnote-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("vnote-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_note (SFI_NOTE_VOID),
		 sfi_pspec_note ("vnote-void", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("note-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("note-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string (NULL),
		 sfi_pspec_string ("string-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string ("test\"string'with\\character-?\007rubbish\177H"),
		 sfi_pspec_string ("string", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string (""),
		 sfi_pspec_string ("string-empty", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 255; i++)
    str256[i] = i + 1;
  str256[i] = 0;
  serialize_cmp (sfi_value_string (str256),
		 sfi_pspec_string ("string-255", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice (NULL),
		 sfi_pspec_choice ("choice-nil", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice ("test-choice-with-valid-characters_9876543210"),
		 sfi_pspec_choice ("choice", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (SFI_MAXINT),
		 sfi_pspec_proxy ("proxy-max", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (G_MAXUINT),
		 sfi_pspec_proxy ("proxy-umax", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bblock (NULL),
		 sfi_pspec_bblock ("bblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  bblock = sfi_bblock_new ();
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 256; i++)
    sfi_bblock_append1 (bblock, i);
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock", NULL, NULL, SFI_PARAM_DEFAULT));
  sfi_bblock_unref (bblock);
  serialize_cmp (sfi_value_fblock (NULL),
		 sfi_pspec_fblock ("fblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  fblock = sfi_fblock_new ();
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
  sfi_fblock_append1 (fblock, -G_MINFLOAT);
  sfi_fblock_append1 (fblock, G_MINFLOAT);
  sfi_fblock_append1 (fblock, -G_MAXFLOAT);
  sfi_fblock_append1 (fblock, G_MAXFLOAT);
  sfi_fblock_append1 (fblock, SFI_MININT);
  sfi_fblock_append1 (fblock, -SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MINNUM);
  sfi_fblock_append1 (fblock, -SFI_MAXNUM);
  sfi_fblock_append1 (fblock, SFI_MAXNUM);
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock", NULL, NULL, SFI_PARAM_DEFAULT));
  
  serialize_cmp (sfi_value_seq (NULL),
		 sfi_pspec_seq ("seq-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  seq = sfi_seq_new ();
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-empty", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  val = sfi_value_bool (TRUE);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_int (42);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_string ("huhu");
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_fblock (fblock);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  if (serial_test_type != SERIAL_TEST_PARAM)
    serialize_cmp (sfi_value_seq (seq),
		   sfi_pspec_seq ("seq-heterogeneous", NULL, NULL,
				  sfi_pspec_real ("dummy", NULL, NULL,
						  0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT),
				  SFI_PARAM_DEFAULT));
  sfi_seq_clear (seq);
  for (i = 0; i < 12; i++)
    {
      val = sfi_value_int (2000 - 3 * i);
      sfi_seq_append (seq, val);
      sfi_value_free (val);
    }
  pspec_homo_seq = sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				  sfi_pspec_int ("integer", NULL, NULL,
						 1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				  SFI_PARAM_DEFAULT);
  sfi_pspec_ref (pspec_homo_seq);
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				sfi_pspec_int ("integer", NULL, NULL,
					       1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				SFI_PARAM_DEFAULT));
  
  if (serial_test_type == SERIAL_TEST_PSPEC)
    {
      serialize_cmp (sfi_value_pspec (NULL),
		     sfi_pspec_pspec ("pspec-nil", NULL, NULL, SFI_PARAM_DEFAULT));
      serialize_cmp (sfi_value_pspec (pspec_homo_seq),
		     sfi_pspec_pspec ("pspec-hseq", NULL, NULL, SFI_PARAM_DEFAULT));
    }

  serialize_cmp (sfi_value_rec (NULL),
		 sfi_pspec_rec ("rec-nil", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  rec = sfi_rec_new ();
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec-empty", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  val = sfi_value_string ("huhu");
  sfi_rec_set (rec, "exo-string", val);
  if (serial_test_type != SERIAL_TEST_PARAM)
    sfi_rec_set (rec, "exo-string2", val);
  sfi_value_free (val);
  val = sfi_value_seq (seq);
  sfi_rec_set (rec, "seq-homogeneous", val);
  sfi_value_free (val);
  val = sfi_value_proxy (102);
  sfi_rec_set (rec, "baz-proxy", val);
  sfi_value_free (val);
  rec_fields.fields = g_new (GParamSpec*, 20); /* should be static mem */
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_proxy ("baz-proxy", NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_string ("exo-string", NULL, NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = pspec_homo_seq;
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  
  sfi_fblock_unref (fblock);
  sfi_seq_unref (seq);
  sfi_pspec_unref (pspec_homo_seq);
  sfi_rec_unref (rec);
  DONE ();
}

static void
test_notes (void)
{
  gchar *str, *error = NULL;
  guint i;
  MSG ("Notes:");
  str = sfi_note_to_string (SFI_MIN_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_MIN_NOTE);
  ASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_KAMMER_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_KAMMER_NOTE);
  ASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_MAX_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_MAX_NOTE);
  ASSERT (error == NULL);
  g_free (str);
  for (i = SFI_MIN_NOTE; i <= SFI_MAX_NOTE; i++)
    {
      gint octave;
      guint semitone;
      gboolean black_semitone;
      gchar letter;
      
      sfi_note_examine (i, &octave, &semitone, &black_semitone, &letter);
      ASSERT (octave == SFI_NOTE_OCTAVE (i));
      ASSERT (semitone == SFI_NOTE_SEMITONE (i));
      ASSERT (SFI_NOTE_GENERIC (octave, semitone) == i);
    }
  sfi_note_from_string_err ("NeverNote", &error);
  ASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
  DONE ();
}

static void
test_time (void)
{
  SfiTime t;
  gchar *error = NULL;
  gchar *str;
  
  MSG ("Time:");
  ASSERT (SFI_USEC_FACTOR == 1000000);
  ASSERT (SFI_MIN_TIME + 1000000 < SFI_MAX_TIME);
  t = sfi_time_system ();
  if (t < SFI_MIN_TIME || t > SFI_MAX_TIME)
    {
      XTICK ();
      t = SFI_MIN_TIME / 2 + SFI_MAX_TIME / 2;
    }
  else
    TICK ();
  t /= SFI_USEC_FACTOR;
  t *= SFI_USEC_FACTOR;
  str = sfi_time_to_string (t);
  ASSERT (sfi_time_from_string_err (str, &error) == t);
  ASSERT (error == NULL);
  g_free (str);
  /* test error returns */
  ASSERT (sfi_time_from_string_err ("foo 22", &error) == 0);
  ASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
  DONE ();
}

static void
test_renames (void)
{
  gchar *str;
  MSG ("Renames:");
  str = g_type_name_to_cname ("PrefixTypeName");
  ASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  str = g_type_name_to_sname ("PrefixTypeName");
  ASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cupper ("PrefixTypeName");
  ASSERT (strcmp (str, "PREFIX_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_type_macro ("PrefixTypeName");
  ASSERT (strcmp (str, "PREFIX_TYPE_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_sname ("prefix_type_name");
  ASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cname ("prefix-type-name");
  ASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  DONE ();
}

static gboolean vmarshal_switch = TRUE;
static guint    vmarshal_count = 0;

static void
generate_vmarshal (guint sig)
{
  gchar *s, mid[32 * 4 + 1];
  guint i, n;
  vmarshal_count++;
  s = mid;
  for (i = sig; i; i >>= 2)
    *s++ = '0' + (i & 3);
  *s = 0;
  s = mid;
  if (!vmarshal_switch)
    {
      g_print ("static void /* %u */\nsfi_vmarshal_%s (gpointer func, gpointer arg0, Arg *alist)\n{\n",
	       vmarshal_count, s);
      g_print ("  void (*f) (gpointer");
      for (i = sig; i; i >>= 2)
	switch (i & 3)
	  {
	  case 1:	g_print (", guint32");		break;
	  case 2:	g_print (", guint64");		break;
	  case 3:	g_print (", double");		break;
	  }
      g_print (", gpointer) = func;\n");
      g_print ("  f (arg0");
      for (i = sig, n = 0; i; i >>= 2, n++)
	switch (i & 3)
	  {
	  case 1:	g_print (", alist[%u].v32", n);		break;
	  case 2:	g_print (", alist[%u].v64", n);		break;
	  case 3:	g_print (", alist[%u].vdbl", n);	break;
	  }
      g_print (", alist[%u].vpt);\n}\n", n);
    }
  else
    g_print ("    case 0x%03x: return sfi_vmarshal_%s; /* %u */\n", sig, s, vmarshal_count);
}

static void
generate_vmarshal_loop (void)
{
  guint sig, i, ki[SFI_VCALL_MAX_ARGS + 1];
  vmarshal_count = 0;
  /* initialize digits */
  for (i = 0; i < SFI_VCALL_MAX_ARGS; i++)
    ki[i] = 1;
  /* initialize overflow */
  ki[SFI_VCALL_MAX_ARGS] = 0;
  while (ki[SFI_VCALL_MAX_ARGS] == 0)	/* abort on overflow */
    {
      /* construct signature */
      sig = 0;
      for (i = 0; i < SFI_VCALL_MAX_ARGS; i++)
	{
	  sig <<= 2;
	  sig |= ki[i];
	}
      /* generate */
      generate_vmarshal (sig);
      /* increase digit wise: 1, 2, 3, 11, 12, 13, 21, 22, 23, 31, ... */
      for (i = 0; ; i++)
	{
	  if (++ki[i] <= 3)
	    break;
	  ki[i] = 1;
	}
    }
}

static void
generate_vmarshal_code (void)
{
  vmarshal_switch = FALSE;
  generate_vmarshal_loop ();

  vmarshal_switch = TRUE;
  g_print ("static VCall\nsfi_vmarshal_switch (guint sig)\n{\n");
  g_print ("  switch (sig)\n    {\n");
  generate_vmarshal_loop ();
  g_print ("    default: g_assert_not_reached (); return NULL;\n");
  g_print ("    }\n}\n");
}

static gchar *pointer1 = "huhu";
static gchar *pointer2 = "haha";
static gchar *pointer3 = "zoot";

static void
test_vcalls_func4 (gpointer o,
		   SfiReal  r,
		   SfiNum   n,
		   gpointer data)
{
  ASSERT (o == pointer1);
  ASSERT (r == -426.9112e-267);
  ASSERT (n == -2598768763298128732);
  ASSERT (data == pointer3);
}

static void
test_vcalls_func7 (gpointer o,
		   SfiReal  r,
		   SfiNum   n,
		   SfiProxy p,
		   SfiInt   i,
		   SfiNum   self,
		   gpointer data)
{
  ASSERT (o == pointer1);
  ASSERT (r == -426.9112e-267);
  ASSERT (n == -2598768763298128732);
  ASSERT (p == (SfiProxy) pointer2);
  ASSERT (i == -2134567);
  ASSERT (self == (SfiNum) test_vcalls_func7);
  ASSERT (data == pointer3);
}

static void
test_vcalls (void)
{
  SfiSeq *seq = sfi_seq_new ();
  MSG ("VCalls:");
  sfi_seq_append_real (seq, -426.9112e-267);
  sfi_seq_append_num (seq, -2598768763298128732);
  sfi_vcall_void (test_vcalls_func4, pointer1,
		  seq->n_elements, seq->elements,
		  pointer3);
  sfi_seq_append_proxy (seq, (SfiProxy) pointer2);
  sfi_seq_append_int (seq, -2134567);
  sfi_seq_append_num (seq, (SfiNum) test_vcalls_func7);
  sfi_vcall_void (test_vcalls_func7, pointer1,
		  seq->n_elements, seq->elements,
		  pointer3);
  DONE ();
  sfi_seq_unref (seq);
}

static void
test_sfidl_seq (void)
{
  TestPositionSeq* pseq;
  TestPosition* pos;
  TestPosition* pos2;
  SfiRec* rec;
  GValue* value;
  MSG ("Sfidl generated code:");

  /* test that types are registered properly */
  ASSERT (TEST_TYPE_POSITION != 0);
  ASSERT (TEST_TYPE_POSITION_SEQ != 0);
  ASSERT (TEST_TYPE_YES_NO_UNDECIDED != 0);

  /* test sequences and structs generated for Position record */
  pseq = test_position_seq_new ();
  ASSERT (pseq != NULL);
  ASSERT (pseq->n_positions == 0);

  pos = test_position_new ();
  ASSERT (pos != NULL);
  pos->x = 1.0;
  pos->y = -1.0;
  pos->relevant = TEST_NO;

  test_position_seq_append (pseq, pos);
  ASSERT (pseq->n_positions == 1);

  test_position_seq_resize (pseq, 4);
  ASSERT (pseq->n_positions == 4);

  test_position_seq_resize (pseq, 1);
  ASSERT (pseq->n_positions == 1);

  rec = test_position_to_rec (pos);
  value = sfi_rec_get (rec, "relevant");

  ASSERT (SFI_VALUE_HOLDS_CHOICE (value));
  ASSERT (strcmp (sfi_value_get_choice (value), "TEST_NO") == 0);

  pos2 = test_position_from_rec (rec);

  ASSERT (pos->x == pos2->x);
  ASSERT (pos->y == pos2->y);
  ASSERT (pos->relevant == pos2->relevant);

  sfi_rec_unref (rec);
  test_position_seq_free (pseq);
  test_position_free (pos);
  test_position_free (pos2);

  /* test validation and defaulting */
  {
    GParamSpec *pspec;
    GValue rec_value = { 0, }, pos_value = { 0, };

    /* create empty record */
    g_value_init (&rec_value, SFI_TYPE_REC);
    sfi_value_take_rec (&rec_value, sfi_rec_new ());

    /* validate record against pspec */
    pspec = sfi_pspec_rec ("foo", "bar", "bazz", test_position_fields, SFI_PARAM_DEFAULT);
    g_param_value_validate (pspec, &rec_value);
    g_param_spec_unref (pspec);

    /* transform record to boxed type */
    g_value_init (&pos_value, TEST_TYPE_POSITION);
    ASSERT (g_value_type_transformable (SFI_TYPE_REC, TEST_TYPE_POSITION));
    g_value_transform (&rec_value, &pos_value);

    /* get boxed type */
    ASSERT (G_VALUE_HOLDS (&pos_value, TEST_TYPE_POSITION));
    pos = g_value_get_boxed (&pos_value);

    /* check that values match defaults */
    ASSERT (pos->x == 2.0);
    ASSERT (pos->y == 3.0);
    ASSERT (pos->relevant == TEST_YES);

    /* cleanup */
    g_value_unset (&rec_value);
    g_value_unset (&pos_value);
  }

  /* test constants */
  ASSERT (TEST_ANSWER_B == 42);
  ASSERT (strcmp(TEST_ULTIMATE_ANSWER, "the answer to all questions is 42") == 0);
  DONE ();
}

#include "testidl.c"

int
main (int   argc,
      char *argv[])
{
  g_thread_init (NULL);
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  
  sfi_init ();
  test_types_init ();

  if (0)
    {
      generate_vmarshal_code ();
      return 0;
    }
  
  test_notes ();
  test_time ();
  test_renames ();
  test_scanner64 ();
  test_typed_serialization (SERIAL_TEST_PARAM);
  test_typed_serialization (SERIAL_TEST_TYPED);
  test_typed_serialization (SERIAL_TEST_PSPEC);
  test_vcalls ();
  test_com_ports ();
  test_threads ();
  test_sfidl_seq ();
  test_misc ();
  
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */

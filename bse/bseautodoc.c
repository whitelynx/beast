/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bse.h"
#include        "../PKG_config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>

static gchar*
type_name (GParamSpec *pspec)
{
  switch (sfi_categorize_pspec (pspec))
    {
    case SFI_SCAT_BOOL:		return g_strdup ("SfiBool");
    case SFI_SCAT_INT:		return g_strdup ("SfiInt");
    case SFI_SCAT_NUM:		return g_strdup ("SfiNum");
    case SFI_SCAT_REAL:		return g_strdup ("SfiReal");
    case SFI_SCAT_STRING:	return g_strdup ("const gchar*");
    case SFI_SCAT_CHOICE:	return g_strdup ("SfiChoice");
    case SFI_SCAT_BBLOCK:	return g_strdup ("SfiBBlock*");
    case SFI_SCAT_FBLOCK:	return g_strdup ("SfiFBlock*");
    case SFI_SCAT_PSPEC:	return g_strdup ("GParamSpec*");
    case SFI_SCAT_SEQ:		return g_strdup ("SfiSeq*");
    case SFI_SCAT_REC:		return g_strdup ("SfiRec*");
    case SFI_SCAT_PROXY:	return g_strdup ("SfiProxy");
    case SFI_SCAT_NOTE:		return g_strdup ("SfiInt");
    case SFI_SCAT_TIME:		return g_strdup ("SfiNum");
    case 0:
      if (G_IS_PARAM_SPEC_ENUM (pspec))
	{
	  GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);
	  return g_strdup (G_OBJECT_CLASS_NAME (espec->enum_class));
	}
      else if (G_IS_PARAM_SPEC_BOXED (pspec))
	return g_strconcat (g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)), "*", NULL);
      else if (G_IS_PARAM_SPEC_OBJECT (pspec))
	return g_strconcat (g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)), "*", NULL);
      /* fall through */
    default:
      g_error ("unhandled pspec type: %s", g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      break;
    }
  return NULL;
}

static void
show_procdoc (void)
{
  BseCategorySeq *cseq;
  guint i;
  
  g_print ("\\input texinfo\n"
	   "@c %%**start of header\n"
	   "@settitle BSE-Procedures\n"
	   "@footnotestyle end\n"
	   "@c %%**end of header\n"
	   "\n"
	   "@include teximacros.texi\n"
	   "\n"
	   "@docpackage{BEAST-%s}\n"
	   "@revision{}\n"
	   "\n"
	   "@unnumbered NAME\n"
	   "@ref_docname{BSE-Procedures - BSE Procedures Reference}\n"
	   "\n"
	   "@unnumbered SYNOPSIS\n"
	   "@printplainindex cp\n"
	   "\n"
	   "@unnumbered DESCRIPTION\n"
	   "\n",
	   BST_VERSION);
  
  cseq = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE);
  for (i = 0; i < cseq->n_cats; i++)
    {
      BseProcedureClass *class = g_type_class_ref (g_type_from_name (cseq->cats[i]->type));
      gchar *cname = g_type_name_to_cname (cseq->cats[i]->type);
      gchar *sname = g_type_name_to_sname (cseq->cats[i]->type);
      guint j;
      
      /* g_print ("\n"); */
      g_print ("@c --\n");
      
      g_print ("@cpindex @ref_function{%s} (", cname);
      for (j = 0; j < class->n_in_pspecs; j++)
	{
	  GParamSpec *pspec = G_PARAM_SPEC (class->in_pspecs[j]);
	  if (j)
	    g_print (", ");
	  g_print ("@ref_parameter{%s}", pspec->name);
	}
      g_print (");\n");
      
      g_print ("@ref_start{}@ref_function{%s} (", cname);
      for (j = 0; j < class->n_in_pspecs; j++)
	{
	  GParamSpec *pspec = G_PARAM_SPEC (class->in_pspecs[j]);
	  if (j)
	    g_print (", ");
	  g_print ("@ref_parameter{%s}", pspec->name);
	}
      g_print (");\n");
      
      g_print ("@ref_start{}(@ref_function{%s} ", sname);
      for (j = 0; j < class->n_in_pspecs; j++)
	{
	  GParamSpec *pspec = G_PARAM_SPEC (class->in_pspecs[j]);
	  gchar *sarg = g_type_name_to_sname (pspec->name);
	  if (j)
	    g_print (" ");
	  g_print ("@ref_parameter{%s}", sarg);
	  g_free (sarg);
	}
      g_print (")\n");
      
      if (class->n_in_pspecs + class->n_out_pspecs)
	{
	  g_print ("@multitable @columnfractions .3 .3 .3\n");
	  for (j = 0; j < class->n_in_pspecs; j++)
	    {
	      GParamSpec *pspec = G_PARAM_SPEC (class->in_pspecs[j]);
	      gchar *tname = type_name (pspec);
	      const gchar *blurb = g_param_spec_get_blurb (pspec);
	      g_print ("@item @ref_type{%s} @tab @ref_parameter{%s} @tab %s\n",
		       tname, pspec->name, blurb ? blurb : "");
	      g_free (tname);
	    }
	  if (class->n_out_pspecs)
	    g_print ("@item @ref_returns{RETURNS:} @tab @tab\n");
	  for (j = 0; j < class->n_out_pspecs; j++)
	    {
	      GParamSpec *pspec = G_PARAM_SPEC (class->out_pspecs[j]);
	      gchar *tname = type_name (pspec);
	      const gchar *blurb = g_param_spec_get_blurb (pspec);
	      g_print ("@item @ref_type{%s} @tab @ref_parameter{%s} @tab %s\n",
		       tname, pspec->name, blurb ? blurb : "");
	      g_free (tname);
	    }
	  g_print ("@end multitable\n");
	}
      
      if (class->blurb)
	g_print ("@ref_blurb{BLURB:} %s\n\n", class->blurb);
      
      if (class->help)
	g_print ("%s\n", class->help);
      
      g_type_class_unref (class);
      g_free (cname);
      g_free (sname);
    }
  bse_category_seq_free (cseq);
  
  g_print ("@revision{}\n");
}

static gint
help (const gchar *name,
      const gchar *arg)
{
  if (arg)
    fprintf (stderr, "%s: unknown argument: %s\n", name, arg);
  fprintf (stderr, "usage: %s [-h]\n", name);
  fprintf (stderr, "       -p       include plugins\n");
  fprintf (stderr, "       -s       include scripts\n");
  fprintf (stderr, "       -h       show help\n");
  
  return arg != NULL;
}

int
main (gint   argc,
      gchar *argv[])
{
  guint i;
  
  g_thread_init (NULL);
  
  bse_init (&argc, &argv, NULL);
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp ("-p", argv[i]) == 0)
	{
	  bsw_register_plugins (NULL, TRUE, NULL, NULL, NULL);
	}
      else if (strcmp ("-s", argv[i]) == 0)
	{
	  bsw_register_scripts (NULL, TRUE, NULL, NULL, NULL);
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (argv[0], NULL);
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  return help (argv[0], NULL);
	}
      else
	return help (argv[0], argv[i]);
    }
  
  show_procdoc ();
  
  return 0;
}

/* SFK - Synthesis Fusion Kit
 * Copyright (C) 2002 Stefan Westerfeld
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
#include <bse.h>
#include <bseglue.h>
#include <sfi/sfiglue.h>
#include <stdio.h>
#include <string>

string removeBse (const string& name)
{
  if (strncmp (name.c_str(), "Bse", 3) == 0 || strncmp (name.c_str(), "Bsw", 3) == 0)
    return name.substr (3);
  else
    return name;
}

string getInterface (const string& name)
{
  int i = name.find ("+", 0);

  if(i >= 0)
  {
    string result = name.substr (0, i);
    if (strncmp (result.c_str(), "Bse", 3) == 0 || strncmp (result.c_str(), "Bsw", 3) == 0)
      result = name.substr (3, i-3);

    return result;
  }
  return "";
}

string getMethod (const string& name)
{
  string result;
  string::const_iterator ni = name.begin ();

  int pos = name.find ("+", 0);
  if (pos >= 0)
    ni += pos + 1;
  else if (name.size () > 4)
    ni += 4; /* assume & skip bs(e|w) prefix */

  while (ni != name.end ())
  {
    if (*ni == '-')
      result += '_';
    else
      result += *ni;
    ni++;
  }
  return result;
}

string signalName (const string& signal)
{
  string result;
  string::const_iterator si = signal.begin ();

  while (si != signal.end ())
  {
    if (*si == '-')
      result += '_';
    else
      result += *si;
    si++;
  }
  return result;
}

string paramName (const string& name)
{
  string result;
  string::const_iterator ni = name.begin ();
  while (ni != name.end ())
  {
    if (*ni == '-')
      result += '_';
    else
      result += *ni;
    ni++;
  }
  return result;
}

string activeInterface = "";
int indent = 0;

void printIndent ()
{
  for (int i = 0; i < indent; i++)
    printf("  ");
}

void setActiveInterface (const string& x, const string& parent)
{
  if (activeInterface != x)
  {
    if (activeInterface != "")
    {
      indent--;
      printIndent ();
      printf ("};\n");
    }

    activeInterface = x;

    if (activeInterface != "")
    {
      printIndent ();
      printf ("class %s", activeInterface.c_str ());
      if (parent != "")
	printf (" : %s", parent.c_str());
      printf (" {\n");
      indent++;
    }
  }
}

string idlType (GType g)
{
  string s = g_type_name (g);

  if (s[0] == 'B' && s[1] == 's' && (s[2] == 'e' || s[2] == 'w'))
    return s.substr(3, s.size() - 3);
  if (s == "guint" || s == "gint" || s == "gulong")
    return "Int";
  if (s == "gchararray")
    return "String";
  if (s == "gfloat" || s == "gdouble")
    return "Real";
  if (s == "gboolean")
    return "Bool";
  if (s == "SfiFBlock")
    return "FBlock";
  return "*" + s + "*";
}

void printPSpec (const char *dir, GParamSpec *pspec)
{
  string pname = paramName (pspec->name);

  printIndent ();
  printf ("%-4s%-20s@= (\"%s\", \"%s\", ",
      dir,
      pname.c_str(),
      g_param_spec_get_nick (pspec) ?  g_param_spec_get_nick (pspec) : "",
      g_param_spec_get_blurb (pspec) ?  g_param_spec_get_blurb (pspec) : ""
      );

  if (SFI_IS_PSPEC_INT (pspec))
  {
    int default_value, minimum, maximum, stepping_rate;

    default_value = sfi_pspec_get_int_default (pspec);
    sfi_pspec_get_int_range (pspec, &minimum, &maximum, &stepping_rate);

    printf("%d, %d, %d, %d, ", default_value, minimum, maximum, stepping_rate);
  }
  if (SFI_IS_PSPEC_BOOL (pspec))
  {
    GParamSpecBoolean *bspec = G_PARAM_SPEC_BOOLEAN (pspec);

    printf("%s, ", bspec->default_value?"TRUE":"FALSE");
  }
  printf("\":flagstodo\");\n");
}

void printMethods (const string& iface)
{
  BseCategorySeq *cseq;
  guint i;

  cseq = bse_categories_match_typed ("*", BSE_TYPE_PROCEDURE);
  for (i = 0; i < cseq->n_cats; i++)
    {
      GType type_id = g_type_from_name (cseq->cats[i]->type);
      BseProcedureClass *klass = (BseProcedureClass *)g_type_class_ref (type_id);

      /* procedures */
      string t = cseq->cats[i]->type;
      string iname = getInterface (t);
      string mname = getMethod (t);
      string rtype = klass->n_out_pspecs ?
	             idlType (klass->out_pspecs[0]->value_type) : "void";

      if (iname == iface)
	{
	  /* for methods, the first argument is implicit: the object itself */
	  guint first_p = (iface == "")?0:1;

	  printIndent ();
	  printf ("%s %s (", rtype.c_str(), mname.c_str ());
	  for (guint p = first_p; p < klass->n_in_pspecs; p++)
	    {
	      string ptype = idlType (klass->in_pspecs[p]->value_type);
	      string pname = paramName (klass->in_pspecs[p]->name);
	      if (p != first_p) printf(", ");
	      printf ("%s %s", ptype.c_str(), pname.c_str());
	    }
	  printf(") {\n");
	  indent++;

	  for (guint p = 0; p < klass->n_out_pspecs; p++)
	    printPSpec ("out", klass->out_pspecs[p]);

	  for (guint p = first_p; p < klass->n_in_pspecs; p++)
	    printPSpec ("in", klass->in_pspecs[p]);

	  indent--;
	  printIndent ();
	  printf ("}\n");
	}
      g_type_class_unref (klass);
    }
  bse_category_seq_free (cseq);
}

/* FIXME: we might want to have a sfi_glue_iface_parent () method */
void printInterface (const string& iface, const string& parent = "")
{
  string idliface = removeBse (iface);

  setActiveInterface (idliface, parent);
  printMethods (idliface);

  if (iface != "")
    {
      /* signals */
      GType type_id = g_type_from_name (iface.c_str());
      if (G_TYPE_IS_INSTANTIATABLE (type_id))
	{
	  guint n_sids;
	  guint *sids = g_signal_list_ids (type_id, &n_sids);
	  for (guint s = 0; s < n_sids; s++)
	    {
	      GSignalQuery query;
	      g_signal_query (sids[s], &query);
	      printIndent();
	      printf ("signal %s (", signalName (query.signal_name).c_str());
	      for (guint p = 0; p < query.n_params; p++)
		{
		  string ptype = idlType (query.param_types[p]);
		  string pname = ""; pname += char('a' + p);
		  if (p != 0) printf(", ");
		  printf ("%s %s", ptype.c_str(), pname.c_str());
	      }
	      printf(");\n");
	    }
	}
      else
	{
	  printf("/* type %s (%s) is not intantiable */\n", g_type_name (type_id), iface.c_str());
	}

      gchar **children = sfi_glue_iface_children (iface.c_str());
      while (*children) printInterface (*children++, idliface);
    }
}

int main (int argc, char **argv)
{
  g_thread_init (NULL);
  bse_init (&argc, &argv, NULL);

  sfi_glue_context_push (bse_glue_context ());
  string s = sfi_glue_base_iface ();

  printf ("namespace Sfk {\n");
  indent++;
  printInterface (s);
  printInterface ("");  /* prints procedures without interface */
  indent--;
  printf ("};\n");

  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */

/* SFI - Synthesis Fusion Kit Interface
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
#include "sfidl-options.h"
#include <glib-extra.h>
#include <stdio.h>

using namespace Sfidl;

static Options *Options_the = 0;

Options *Options::the() {
  g_return_val_if_fail (Options_the != 0, 0);

  return Options_the;
};

Options::Options ()
{
  generateExtern = generateData = generateConstant = false;
  generateTypeH = generateTypeC = false;
  generateBoxedTypes = generateProcedures = generateSignalStuff = false;
  generateIdlLineNumbers = false;
  targetCore = targetC = targetQt = false;
  doHeader = doImpl = doHelp = false;
  sfidlName = "sfidl";

  Options_the = this;
}

bool Options::parse (int *argc_p, char **argv_p[])
{
  unsigned int argc;
  char **argv;
  unsigned int i, e;

  g_return_val_if_fail (argc_p != NULL, false);
  g_return_val_if_fail (argv_p != NULL, false);
  g_return_val_if_fail (*argc_p >= 0, false);

  argc = *argc_p;
  argv = *argv_p;

  if (argc && argv[0])
    sfidlName = argv[0];

  for (i = 1; i < argc; i++)
    {
      unsigned int len = 0;

      if (strcmp ("--sfk-core-header", argv[i]) == 0)
	{
	  targetCore = true;
	  doHeader = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--sfk-core-impl", argv[i]) == 0)
	{
	  targetCore = true;
	  doImpl = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--c-client-header", argv[i]) == 0)
	{
	  targetC = true;
	  doHeader = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--c-client-impl", argv[i]) == 0)
	{
	  targetC = true;
	  doImpl = true;
	  argv[i] = NULL;
	}
      else if ((len = strlen("--c-client-prefix=")) &&
	       (strcmp ("--c-client-prefix", argv[i]) == 0 ||
	       strncmp ("--c-client-prefix=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;
	  
	  if (*equal == '=')
	    prefixC = equal + 1;
	  else if (i + 1 < argc)
	    {
	      prefixC = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--qt-client-header", argv[i]) == 0)
	{
	  targetQt = true;
	  doHeader = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--qt-client-impl", argv[i]) == 0)
	{
	  targetQt = true;
	  doImpl = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--help", argv[i]) == 0)
	{
	  doHelp = true;
	  argv[i] = NULL;
	}
      else if (strcmp ("--idl-line-numbers", argv[i]) == 0)
	{
	  generateIdlLineNumbers = true;
	  argv[i] = NULL;
	}
      else if ((len = strlen("--qt-client-namespace=")) &&
	       (strcmp ("--qt-client-namespace", argv[i]) == 0 ||
	       strncmp ("--qt-client-namespace=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;
	  
	  if (*equal == '=')
	    namespaceQt = equal + 1;
	  else if (i + 1 < argc)
	    {
	      namespaceQt = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if ((len = strlen("--init=")) &&
	       (strcmp ("--init", argv[i]) == 0 ||
	       strncmp ("--init=", argv[i], len) == 0))
	{
	  char *equal = argv[i] + len;
	  
	  if (*equal == '=')
	    initFunction = equal + 1;
	  else if (i + 1 < argc)
	    {
	      initFunction = argv[i + 1];
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if ((len = strlen("-I")) &&
	       (strcmp ("-I", argv[i]) == 0 ||
	       strncmp ("-I", argv[i], len) == 0))
	{
	  char *path = argv[i] + len;
	  const char *dir = 0;
	  
	  if (*path != 0)
	    includePath.push_back (path);
	  else if (i + 1 < argc)
	    {
	      includePath.push_back (argv[i + 1]);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
    }

  /* resort argc/argv */
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

  /* option validation */
  if (((targetCore?1:0) + (targetQt?1:0) + (targetC?1:0)) > 1)
    {
      fprintf(stderr, "you can only generate code for one target at a time\n");
      return false;
    }

  generateBoxedTypes = targetCore;

  if (doHeader)
    {
      generateTypeH = true;
      generateConstant = true;

      if (targetCore)
	generateExtern = true;
    }

  if (doImpl)
    {
      generateTypeC = true;

      if (targetQt || targetC)
	generateProcedures = true;
    }

  if (targetCore && doImpl)
    {
      generateData = true;
      if (initFunction == "")
	{
	  fprintf (stderr, "you need to specify an init function name\n");
	  return false;
	}
    }
  else
    {
      if (initFunction != "")
	{
	  fprintf (stderr, "you don't need to specify an init function name\n");
	  initFunction = "";
	}
    }

  return true;
}

void Options::printUsage ()
{
  fprintf(stderr, "usage: %s [ <options> ] <idlfile>\n", sfidlName.c_str());
  fprintf(stderr, "\n");
  fprintf(stderr, "options for the C language binding:\n");
  fprintf(stderr, " --sfk-core-header           generate c header to use within the sfk core\n");
  fprintf(stderr, " --sfk-core-impl             generate c source to use within the sfk core\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " --c-client-header           generate c header for clients using sfk\n");
  fprintf(stderr, " --c-client-impl             generate c source for clients using sfk\n");
  fprintf(stderr, " --c-client-prefix <prefix>  set the prefix for c functions\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "options for the C++ language binding:\n");
  fprintf(stderr, " --qt-client-header          generate C++ header to interface sfk using Qt\n");
  fprintf(stderr, " --qt-client-impl            generate C++ source to interface sfk using Qt\n");
  fprintf(stderr, " --qt-client-namespace <ns>  set the namespace to use for the code\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "options for both:\n");
  fprintf(stderr, " --init <name>               set the name of the init function\n");
  fprintf(stderr, " --idl-line-numbers          generate #line directives relative to .sfidl file\n");
  fprintf(stderr, " -I <directory>              add this directory to the include path\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " --help                      this help\n");
}


/* vim:set ts=8 sts=2 sw=2: */

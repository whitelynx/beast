/* BirnetOSUnix
 * Copyright (C) 2009 Stefan Westerfeld
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
#include "birnetos.hh"
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <setjmp.h>
#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

namespace Birnet {
namespace OS {

int
get_thread_priority (int thread_id)
{
  return getpriority (PRIO_PROCESS, thread_id);
}

static jmp_buf check_sse_sys_jmp_buf;

static void BIRNET_NORETURN
check_sse_sys_sigill_handler (int dummy)
{
  longjmp (check_sse_sys_jmp_buf, 1);
}

bool
check_sse_sys()
{
  struct sigaction action, old_action;
  bool x86_ssesys = false;

  action.sa_handler = check_sse_sys_sigill_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = SA_NOMASK;
  sigaction (SIGILL, &action, &old_action);
  if (setjmp (check_sse_sys_jmp_buf) == 0)
    {
      unsigned int mxcsr;
      __asm__ __volatile__ ("stmxcsr %0 ; sfence ; emms" : "=m" (mxcsr));
      /* executed SIMD instructions without exception */
      x86_ssesys = true;
    }
  else
    {
      /* signal handler jumped here */
      // g_printerr ("caught SIGILL\n");
    }
  sigaction (SIGILL, &old_action, NULL);
  return x86_ssesys;
}

void
raise_sigtrap ()
{
  raise (SIGTRAP);
}

void
memset4 (guint32   *mem,
         guint32    filler,
         guint      count) 
{
  BIRNET_STATIC_ASSERT (sizeof (*mem) == 4);
  BIRNET_STATIC_ASSERT (sizeof (filler) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) mem, filler, count);
}

void
memcpy4 (uint32       *dest,
         const uint32 *src,
         size_t        count)
{
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemcpy ((wchar_t*) dest, (const wchar_t*) src, count);
}

// (at least) 48-bit random number generator
void
srand48 (int32 seedval)
{
  ::srand48 (seedval);
}

int32
lrand48()
{
  return ::lrand48();
}

int
getpid()
{
  return ::getpid();
}

int
strerror_r (int errnum, char *buf, size_t buflen)
{
  char *result = ::strerror_r (errnum, buf, buflen);
  if (!result)
    {
      errno = EINVAL;
      return -1;
    }
  if (result != buf)
    {
      if (strlen (result) + 1 < buflen)
	{
	  strcpy (buf, result);
	  return 0;
	}
      else
	{
	  errno = ERANGE;
	}
    }
  else
    {
      return 0;
    }
  return -1;
}

void
syslog (int priority, char const* format, ...)
{
  va_list ap;

  va_start(ap, format);
  vsyslog (priority, format, ap);
  va_end(ap);
}

int
lstat (const char  *path,
       struct stat *buf)
{
  return ::lstat (path, buf);
}

bool
stat_is_socket (mode_t mode)
{
  return S_ISSOCK (mode);
}

bool
stat_is_link (mode_t mode)
{
  return S_ISLNK (mode);
}

int
vasprintf (char **strp, const char *fmt, va_list ap)
{
  return ::vasprintf (strp, fmt, ap);
}

int
mkdir (const char *path,
       mode_t      mode)
{
  return ::mkdir (path, mode);
}

bool
url_test_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    bool          asyncronous; /* start asyncronously and check exit code to catch launch errors */
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* configurable, working browser launchers */
    { "gnome-open",             NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    { "exo-open",               NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    /* non-configurable working browser launchers */
    { "kfmclient",              "openURL",      "", "", 0 }, /* opens in background, correct exit_code */
    { "gnome-moz-remote",       "--newwin",     "", "", 0 }, /* opens in background, correct exit_code */
#if 0
    /* broken/unpredictable browser launchers */
    { "browser-config",         NULL,            "", "", 0 }, /* opens in background (+ sleep 5), broken exit_code (always 0) */
    { "xdg-open",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "sensible-browser",       NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "htmlview",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
#endif
    /* direct browser invocation */
    { "x-www-browser",          NULL,           "", "", 1 }, /* opens in foreground, browser alias */
    { "firefox",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla-firefox",        NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "konqueror",              NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "opera",                  "-newwindow",   "", "", 1 }, /* opens in foreground, correct exit_code */
    { "galeon",                 NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "epiphany",               NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "amaya",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "dillo",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
  };
  uint i;
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        char fallback_error[64] = "Ok";
        bool success;
        if (!www_browsers[i].asyncronous) /* start syncronously and check exit code */
          {
            int exit_status = -1;
            success = g_spawn_sync (NULL, /* cwd */
                                    args,
                                    NULL, /* envp */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child_setup() */
                                    NULL, /* user_data */
                                    NULL, /* standard_output */
                                    NULL, /* standard_error */
                                    &exit_status,
                                    &error);
            success = success && !exit_status;
            if (exit_status)
              g_snprintf (fallback_error, sizeof (fallback_error), "exitcode: %u", exit_status);
          }
        else
          success = g_spawn_async (NULL, /* cwd */
                                   args,
                                   NULL, /* envp */
                                   G_SPAWN_SEARCH_PATH,
                                   NULL, /* child_setup() */
                                   NULL, /* user_data */
                                   NULL, /* child_pid */
                                   &error);
        g_free (string);
        Msg::display (debug_browser, "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
        g_clear_error (&error);
        if (success)
          return true;
        www_browsers[i].disabled = true;
      }
  /* reset all disabled states if no browser could be found */
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

} // OS

// permissions
int OS_S_IXGRP = S_IXGRP;
int OS_S_IXOTH = S_IXOTH;

} // Birnet


/* BirnetOSWin32
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
#include <glib.h>
#include <glib/gprintf.h>
#include <process.h>
#include <sys/stat.h>

namespace Birnet {
namespace OS {

void
raise_sigtrap()
{
  G_BREAKPOINT();
}

int
get_thread_priority (int thread_id)
{
  /* FIXME */
  return 0;
}

bool
check_sse_sys()
{
  return true;
}

void
syslog (int priority, char const* format, ...)
{
  /* FIXME: */
  g_error ("syslog is not supported on windows\n");
}

static inline GRand *
grandom48()
{
  static GRand *r48 = NULL;
  if (BIRNET_UNLIKELY (!r48))
    r48 = g_rand_new();
  return r48;
}

// (at least) 48-bit random number generator
void
srand48 (int32 seedval)
{
  g_rand_set_seed (grandom48(), seedval);
}

int32
lrand48()
{
  return g_rand_int (grandom48());
}

int
getpid()
{
  return _getpid();
}

int
lstat (const char  *path,
       struct stat *buf)
{
  return ::stat (path, buf);
}

bool
stat_is_socket (mode_t mode)
{
  return false;
}

bool
stat_is_link (mode_t mode)
{
  return false;
}

int
vasprintf (char **strp, const char *fmt, va_list ap)
{
  return g_vasprintf (strp, fmt, ap);
}

/* MSDN recommends using strerror_s as threadsafe replacement for strerror, however it seems to
   be impossible to use it from mingw gcc, so we reimplement a threadsafe version
*/
int
strerror_r (int errnum, char *buf, size_t buflen)
{
  if (errnum >= 0 && errnum < _sys_nerr)
    {
      char *err_msg = _sys_errlist[errnum];
      size_t len = strlen (err_msg);
      if (len < buflen)
	{
	  strcpy (buf, err_msg);
	  errno = 0;
	  return 0;
	}
      else
	{
	  errno = ERANGE;
	}
    }
  else
    {
      errno = EINVAL;
    }
  return -1;
}

void
memset4 (uint32 *mem, uint32 filler, uint length)
{
  while (length--)
    *mem++ = filler;
}

int
mkdir (const char *path,
       mode_t      mode)
{
  return ::mkdir (path);
}

} // OS

/* fake errno values */
int OS_ENOTBLK  = 1000;
int OS_ENOTSOCK = 1001;

/* fake permissions */
int OS_S_IXOTH = 0;
int OS_S_IXGRP = 0;

} // Birnet
/* vim:set ts=8 sts=2 sw=2: */

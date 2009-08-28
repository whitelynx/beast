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
         guint      length) 
{
  BIRNET_STATIC_ASSERT (sizeof (*mem) == 4);
  BIRNET_STATIC_ASSERT (sizeof (filler) == 4);
  BIRNET_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) mem, filler, length);
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

} // OS

// permissions
int OS_S_IXGRP = S_IXGRP;
int OS_S_IXOTH = S_IXOTH;

// some errno values are not supported by every system
int OS_ENOTBLK = ENOTBLK;
int OS_ENOTSOCK = ENOTSOCK;


} // Birnet


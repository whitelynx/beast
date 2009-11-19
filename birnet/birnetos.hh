/* BirnetOS
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
#ifndef __BIRNET_OS_HH__
#define __BIRNET_OS_HH__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// some errno values are not supported by every system
enum {
  BIRNET_OS_ENOTBLK
#ifdef ENOTBLK
                    = ENOTBLK,
#else
                    = 1000,
#endif
  BIRNET_OS_ENOTSOCK
#ifdef ENOTSOCK
                    = ENOTSOCK,
#else
                    = 1001,
#endif
  BIRNET_OS_ELOOP
#ifdef ELOOP
                    = ELOOP,
#else
                    = 1002,
#endif
  BIRNET_OS_ENOMSG
#ifdef ENOMSG
                    = ENOMSG,
#else
                    = 1003,
#endif
  BIRNET_OS_ETXTBSY
#ifdef ETXTBSY
                    = ETXTBSY,
#else
                    = 1004,
#endif
};

#ifdef __cplusplus
#include <birnet/birnetutils.hh>

namespace Birnet {

namespace OS {

/* --- functions --- */
int   get_thread_priority (int thread_id);
bool  check_sse_sys();
void  syslog (int priority, const char *format, ...);
void  raise_sigtrap();
int   getpid();
void  memset4 (uint32 *mem, uint32 filler, uint count); 
void  memcpy4 (uint32 *dest, const uint32 *src, size_t count);
int   mkdir (const char *path, mode_t mode);
bool  url_test_show (const char *url);

// stat
int   lstat (const char *path, struct stat *buf);
bool  stat_is_socket (mode_t mode);        /* S_ISSOCK */
bool  stat_is_link (mode_t mode);          /* S_ISLNK */

// (at least) 48-bit random number generator
void  srand48 (int32 seedval);
int32 lrand48();

// printf formatting
int   vasprintf (char **strp, const char *fmt, va_list ap);

// errno
int   strerror_r (int errnum, char *buf, size_t buflen);

} // OS

// permissions
extern int OS_S_IXGRP;
extern int OS_S_IXOTH;

// some errno values are not supported by every system
enum {
  OS_ENOTBLK = BIRNET_OS_ENOTBLK,
  OS_ENOTSOCK = BIRNET_OS_ENOTSOCK,
  OS_ELOOP = BIRNET_OS_ELOOP,
  OS_ENOMSG = BIRNET_OS_ENOMSG,
  OS_ETXTBSY = BIRNET_OS_ETXTBSY,
};

} // Birnet

#endif /* __cplusplus */

#endif /* __BIRNET_OS_HH__ */
/* vim:set ts=8 sts=2 sw=2: */

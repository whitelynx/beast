/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2001-2002 Tim Janik
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#ifndef	__TOYPROF_H__
#define __TOYPROF_H__

#define _GNU_SOURCE
#include <stdlib.h>	/* to define __GLIBC__ and __GLIBC_MINOR__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if     defined (__GNUC__) && defined (__GLIBC__) &&	/* have GCC && GLIBC */	\
        ((__GNUC__ == 2 && __GNUC_MINOR__ >= 95) ||	/* GCC >= 2.95 */	\
         __GNUC__ > 2) &&               		/* GCC >= 3.0.0 */	\
        ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2) ||    /* GLIBC >= 2.2 */	\
         __GLIBC__ > 2)					/* GLIBC > 2 */

/* function flagging */
#define	TOYPROF_GNUC_NO_INSTRUMENT	__attribute__((no_instrument_function))
#define	TOYPROF_GNUC_UNUSED		__attribute__((unused))

/* print out profiling statistics to filedescriptor */
void toyprof_dump_stats	(int fd) TOYPROF_GNUC_NO_INSTRUMENT;

/* profiling types */
typedef enum	/*< skip >*/
{
  TOYPROF_OFF			= 0x0,
  TOYPROF_PROFILE_TIMING	= 0x1,
  TOYPROF_TRACE_FUNCTIONS	= 0x3
} ToyprofBehaviour;

/* enable/disable profiling and optionally trace functions */
void toyprof_set_profiling (ToyprofBehaviour behav) TOYPROF_GNUC_NO_INSTRUMENT;

#endif	/* >= GCC-2.95 */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __TOYPROF_H__ */

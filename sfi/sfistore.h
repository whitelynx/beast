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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_STORE_H__
#define __SFI_STORE_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- typedefs and structures --- */
typedef guint (*SfiStoreReadBin)	(gpointer	 data,
					 guint64	 pos,
					 void		*buffer,
					 guint		 blength);
typedef struct
{
  GString *text;
  guint	   indent;
  SfiRing *bblocks;
  guint	   needs_break : 1;
} SfiWStore;
typedef enum	/*< skip >*/
{
  SFI_TOKEN_UNMATCHED	= G_TOKEN_LAST + 1,
  SFI_TOKEN_LAST
} SfiTokenType;
typedef struct _SfiRStore SfiRStore;
typedef SfiTokenType (*SfiStoreParser)	(gpointer	 context_data,
					 SfiRStore	*rstore, /* parser_this */
					 GScanner	*scanner,
					 gpointer	 user_data);
struct _SfiRStore
{
  GScanner      *scanner;
  gchar         *fname;
  gpointer       parser_this;
};


/* --- writable store --- */
SfiWStore*	sfi_wstore_new		(void);
void		sfi_wstore_destroy	(SfiWStore	*wstore);
void		sfi_wstore_push_level	(SfiWStore	*wstore);
void		sfi_wstore_pop_level	(SfiWStore	*wstore);
void		sfi_wstore_puts		(SfiWStore	*wstore,
					 const gchar	*string);
void		sfi_wstore_putc		(SfiWStore	*wstore,
					 gchar		 character);
void		sfi_wstore_printf	(SfiWStore	*wstore,
					 const gchar	*format,
					 ...) G_GNUC_PRINTF (2, 3);
void		sfi_wstore_break	(SfiWStore	*wstore);
void		sfi_wstore_put_value	(SfiWStore	*wstore,
					 const GValue	*value);
void		sfi_wstore_put_param	(SfiWStore	*wstore,
					 const GValue	*value,
					 GParamSpec	*pspec);
void		sfi_wstore_put_bin_data (SfiWStore	*wstore,
					 SfiStoreReadBin reader,
					 gpointer	 data,
					 GDestroyNotify	 destroy);
void		sfi_wstore_flush_fd	(SfiWStore	*wstore,
					 gint		 fd);
const gchar*	sfi_wstore_peek_text	(SfiWStore	*wstore,
					 guint		*length);


/* --- readable store --- */
SfiRStore*	sfi_rstore_new		(void);
void		sfi_rstore_destroy	(SfiRStore	*rstore);
void		sfi_rstore_input_fd	(SfiRStore	*rstore,
					 gint		 fd,
					 const gchar	*fname);
void		sfi_rstore_input_text	(SfiRStore	*rstore,
					 const gchar	*text);
gboolean	sfi_rstore_eof		(SfiRStore	*rstore);
GTokenType	sfi_rstore_parse_param  (SfiRStore	*rstore,
					 GValue		*value,
					 GParamSpec	*pspec);
GTokenType	sfi_rstore_parse_rest	(SfiRStore	*rstore,
					 gpointer	 context_data,
					 SfiStoreParser	 try_statement,
					 gpointer	 user_data);
guint		sfi_rstore_parse_all	(SfiRStore	*rstore,
					 gpointer	 context_data,
					 SfiStoreParser	 try_statement,
					 gpointer	 user_data);
void            sfi_rstore_error	(SfiRStore      *rstore,
					 const gchar    *format,
					 ...) G_GNUC_PRINTF (2,3);
void            sfi_rstore_unexp_token	(SfiRStore      *rstore,
					 GTokenType      expected_token);
void            sfi_rstore_warn		(SfiRStore      *rstore,
					 const gchar    *format,
					 ...) G_GNUC_PRINTF (2,3);
GTokenType	sfi_rstore_warn_skip	(SfiRStore      *rstore,
					 const gchar    *format,
					 ...) G_GNUC_PRINTF (2,3);
extern const GScannerConfig *sfi_rstore_scanner_config;


/* --- logging --- */
#define	SFI_LOG_ERROR	(0)
#define	SFI_LOG_WARN	(1)
#define	SFI_LOG_INFO	(2)
#define	SFI_LOG_DEBUG	(3)
static void	sfi_error	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_warn	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_info	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_debug	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
void		sfi_log_valist	(const gchar *log_domain,
				 guint        level,
				 const gchar *format,
				 va_list      args);
/* logging API impls */
static void
sfi_error (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_ERROR, format, args);
  va_end (args);
}
static void
sfi_warn (const gchar *format,
	  ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_WARN, format, args);
  va_end (args);
}
static void
sfi_info (const gchar *format,
	  ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_INFO, format, args);
  va_end (args);
}
#if SFI_DISABLE_DEBUG
static void
sfi_debug (const gchar *format,
	   ...)
{
}
#define	sfi_debug	while (0) sfi_debug
#else
static void
sfi_debug (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_DEBUG, format, args);
  va_end (args);
}
#endif


G_END_DECLS

#endif /* __SFI_STORE_H__ */

/* vim:set ts=8 sts=2 sw=2: */

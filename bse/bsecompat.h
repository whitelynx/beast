/* BSE - Better Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#ifndef __BSE_COMPAT_H__
#define __BSE_COMPAT_H__

#include        <bse/bseenums.h>

G_BEGIN_DECLS


/* --- parameter changes --- */
#define bse_param_spec_genum(name,nick,blurb, genum_type, default_value, hints) \
   bse_param_spec_enum (name,nick,blurb, default_value, genum_type, hints)
#define bse_param_spec_freq_simple(name, nick, blurb, hints) \
  bse_param_spec_freq (name, nick, blurb, BSE_KAMMER_FREQUENCY, BSE_MIN_OSC_FREQUENCY, BSE_MAX_OSC_FREQUENCY, hints)
#define bse_param_spec_fine_tune(name, nick, blurb)	\
  sfi_pspec_int (name, nick, blurb, 0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10, \
                 "scale:" SFI_PARAM_STANDARD)
#define	bse_param_spec_octave(name, nick, blurb)	\
  sfi_pspec_int (name, nick, blurb, BSE_KAMMER_OCTAVE, \
                 BSE_MIN_OCTAVE, BSE_MAX_OCTAVE, 2, SFI_PARAM_STANDARD)
#define bse_pspec_note(name, nick, blurb, default_value, hints) \
  sfi_pspec_note (name, nick, blurb, default_value, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, hints)
#define bse_pspec_note_simple(name, nick, blurb, hints) \
  bse_pspec_note (name, nick, blurb, SFI_KAMMER_NOTE, hints)

/* --- BSE format changes --- */
gchar*  bse_compat_rewrite_type_name            (BseStorage    *storage,
                                                 const gchar   *type_name);
gchar*  bse_compat_rewrite_ichannel_ident       (BseStorage    *storage,
                                                 const gchar   *type_name,
                                                 const gchar   *ichannel_ident);
gchar*  bse_compat_rewrite_ochannel_ident       (BseStorage    *storage,
                                                 const gchar   *type_name,
                                                 const gchar   *ochannel_ident);


G_END_DECLS

#endif /* __BSE_COMPAT_H__ */

# BEAST - Better Audio System
# Copyright (C) 2003 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) $(BSE_CFLAGS) -DG_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS

ZFILE_DEFS = $(strip \
	gus-patch		$(srcdir)/gus-patch.bse		\
	adsr-wave-1		$(srcdir)/adsr-wave-1.bse	\
	adsr-wave-2		$(srcdir)/adsr-wave-2.bse	\
	plain-wave-1		$(srcdir)/plain-wave-1.bse	\
	plain-wave-2		$(srcdir)/plain-wave-2.bse	\
)

gen_sources = xgen-bzc xgen-bzh
CLEANFILES += $(gen_sources) bse-zfile.c bse-zfile.h
noinst_DATA = bse-zfile.c # bse-zfile.h
EXTRA_DIST += $(filter %.bse, $(ZFILE_DEFS))

bse-zfile.c:	$(top_builddir)/birnet/birnet-zintern $(srcdir)/*.bse # bse-zfile.h
	$(top_builddir)/birnet/birnet-zintern -b -z $(ZFILE_DEFS) > xgen-bzc		\
	&& echo "static const BseZFile bse_zfiles[] = {" >> xgen-bzc			\
	&& for i in $(filter-out %.bse, $(ZFILE_DEFS)) ; do				\
		uname=$$(echo $$i | tr a-z- A-Z_);					\
		echo "  { \"$$i\", $$uname""_SIZE, "	>>xgen-bzc;			\
		echo "    $$uname""_DATA, G_N_ELEMENTS ($$uname""_DATA) }," >>xgen-bzc;	\
	done										\
	&& echo "};" >> xgen-bzc							\
	&& cp xgen-bzc $@								\
	&& rm -f xgen-bzc
#bse-zfile.h:	zintern $(srcdir)/*.bse
#	for i in $(filter BSE_%, $(ZFILE_DEFS)) ; do				\
#		echo "#define $$i	\"BSE_$$i\""	>> xgen-bzh;		\
#	done									\
#	&& cp xgen-bzh $@							\
#	&& rm -f xgen-bzh

# need -I$(top_srcdir) for <gsl/gsldefs.h>
# need -I. for "../topconfig.h"

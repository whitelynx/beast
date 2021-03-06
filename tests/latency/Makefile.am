# BSE - Better Sound Engine
# Copyright (C) 2004 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(SFI_CFLAGS)
DEFS     += -DG_LOG_DOMAIN="\"BSE-TEST\"" # -DG_DISABLE_CONST_RETURNS
# IDL Compiler
SFIDL     = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)
BSESCM    = $(top_builddir)/shell/bsescm-$(BIN_VERSION)

# IDL generation rule for plugins
%.genidl.hh: %.idl $(BSEPGEN) $(SFIDL)
	$(SFIDL) $(SFIDL_INC) --plugin --macro $(<F) $< > $@ || (rm $@ ; exit 1 )

#
# Plugin build rules
#
# bselatencytest:
bselatencytest_la_SOURCES = bselatencytest.cc
bselatencytest_la_LDFLAGS = -module -avoid-version
bselatencytest_la_LIBADD  = $(top_builddir)/bse/libbse.la $(BSE_LIBS) -lm
EXTRA_HEADERS += bselatencytest.idl
$(srcdir)/bselatencytest.cc: bselatencytest.idl
$(srcdir)/bselatencytest.cc: bselatencytest.genidl.hh
CLEANFILES += bselatencytest.genidl.hh

# libraries to build
lib_LTLIBRARIES = bselatencytest.la
# prevent libraries from being installed
install-libLTLIBRARIES:

builddirplugins = ./.libs/*.so:$(top_builddir)/plugins/.libs/*.so

# 
# test rule
#
test: 	all
	cd . \
	&& rm -f ./tmp-midi-fifo					\
	&& mkfifo ./tmp-midi-fifo					\
	&& FIFO=tmp-midi-fifo $(BSESCM)					\
	    -m oss=tmp-midi-fifo -p null				\
	   --bse-override-plugin-globs $(builddirplugins)		\
	   --bse-override-script-path $(top_builddir)/shell/		\
	   -s $(srcdir)/bseplay.scm $(srcdir)/midi-latency.bse
CLEANFILES += tmp-midi-fifo

EXTRA_DIST += $(strip 		\
	bseplay.scm		\
	midi-latency.bse	\
	$(EXTRA_HEADERS)	\
)

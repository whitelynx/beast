# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I. $(BIRNET_CFLAGS)
DEFS	       += -DBIRNET_LOG_DOMAIN='"$(basename $(@F))"' -DPARANOID -DG_DISABLE_CONST_RETURNS

noinst_PROGRAMS  = $(ALLTESTS)
progs_ldadd 	 = $(top_builddir)/birnet/libbirnet.o $(BIRNET_LIBS) -lm

TESTS		+= infotest
infotest_SOURCES = infotest.cc
infotest_LDADD	 = $(progs_ldadd)
TESTS		+= math
PERFTESTS       += math
math_SOURCES	 = math.cc
math_LDADD	 = $(progs_ldadd)
TESTS		+= strings
SLOWTESTS       += strings
strings_SOURCES  = strings.cc
strings_LDADD	 = $(progs_ldadd)
TESTS		+= systest
systest_SOURCES  = systest.cc
systest_LDADD	 = $(progs_ldadd)
TESTS		+= threads
PERFTESTS       += threads
threads_SOURCES	 = threads.cc
threads_LDADD	 = $(progs_ldadd)
TESTS		+= sorting
SLOWTESTS       += sorting
sorting_SOURCES  = sorting.cc
sorting_LDADD	 = $(progs_ldadd)
TESTS		+= datalist
datalist_SOURCES = datalist.cc
datalist_LDADD	 = $(progs_ldadd)
TESTS		+= utils
utils_SOURCES    = utils.cc
utils_LDADD	 = $(progs_ldadd)

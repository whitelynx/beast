# SFI - Synthesis Fusion Kit Interface
# Copyright (C) 2002 Tim Janik
#
## Makefile.am for SFI

SUBDIRS =

INCLUDES += -I.. $(SFI_CFLAGS) -I.

# public libraries
lib_LTLIBRARIES = libsfi.la
DEFS += -DG_LOG_DOMAIN=\"SFI\" -DG_DISABLE_CONST_RETURNS
libsfiincludedir = $(includedir)/sfi
libsfi_la_LDFLAGS = $(strip \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic \
)
libsfi_la_LIBADD = $(SFI_LIBS) -lm

sfi_headers = $(strip \
	glib-extra.h	sfi.h		\
	sfivcall.h	sfiglue.h	sfigluecodec.h	sfiglueproxy.h	\
	sfinote.h	sfiparams.h	sfiprimitives.h	sfiserial.h	\
	sfitime.h	sfitypes.h	sfivalues.h	sfiustore.h	\
)
sfi_c_sources = $(strip \
	glib-extra.c	\
	sfivcall.c	sfiglue.c	sfigluecodec.c	sfiglueproxy.c	\
	sfinote.c	sfiparams.c	sfiprimitives.c	sfiserial.c	\
	sfitime.c	sfitypes.c	sfivalues.c	sfiustore.c	\
)
sfi_extra_sources = $(strip \
	gbsearcharray.h \
)
sfi_built_sources = $(strip \
)

$(sfi_c_sources): $(sfi_built_sources)

libsfi_la_SOURCES = $(sfi_c_sources)

#
# generated sources
#
SFIDL = $(top_builddir)/sfi/sfidl
temp_files =  xgen-tih xgen-tic
CLEANFILES += $(temp_files)
CLEANFILES += testidl.h testidl.c
testidl.h: testidl.sfidl sfidl
	$(SFIDL) --sfk-core-header testidl.sfidl > xgen-tih && mv xgen-tih testidl.h
testidl.c: testidl.sfidl sfidl
	$(SFIDL) --sfk-core-impl --init test_types_init testidl.sfidl > xgen-tic && mv xgen-tic testidl.c

#
# programs to build
#
noinst_PROGRAMS = sfidl testsfi
# source files
sfidl_SOURCES = sfidl.cc sfidl-namespace.cc glib-extra.c
progs_extra_sources += sfidl-namespace.h
sfidl_LDADD = $(SFI_LIBS) -lm
testsfi_SOURCES = testsfi.c
testsfi_LDADD = libsfi.la
testsfi.c: testidl.h testidl.c

#
# TESTS
#
TESTS = testsfi


EXTRA_DIST += $(sfi_headers) $(sfi_extra_sources) $(progs_extra_sources) $(sfi_built_sources)
MAINTAINERCLEANFILES += $(sfi_built_sources)

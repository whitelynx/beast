# Birnet
# Copyright (C) 2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS		= . tests
INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BIRNET_CFLAGS)
DEFS            = -DBIRNET_LOG_DOMAIN=\"BIRNET\" -D_BIRNET_SOURCE_EXTENSIONS
GLIB_MKENUMS    = glib-mkenums

birnet_headers = $(strip 		\
	birnet.hh			\
	birnetcdefs.h			\
	birnetcpu.hh			\
	birnetdebugtools.hh		\
	birnetmsg.hh			\
	birnettests.h			\
	birnetthread.hh			\
	birnetmath.hh			\
	birnetutf8.hh			\
	birnetutils.hh			\
)
birnet_sources = $(strip 		\
	birnetthreadimpl.cc		\
	birnetcpu.cc			\
	birnetdebugtools.cc		\
	birnetmsg.cc			\
	birnetthread.cc			\
	birnetmath.cc			\
	birnetutf8.cc			\
	birnetutils.cc			\
)
birnet_private_headers = $(strip 	\
)
birnet_generated_headers = $(strip 	\
	birnetconfig.h			\
)
EXTRA_DIST += configure.inc acbirnet.m4

# Birnet library
# we want a partial shared library here. libtool creates that if the library
# name ends in .lo or .o. however automake doesn't know .lo or .o libraries,
# so we simply build an ordinary non-installed .la library and then use our
# own rule to build the .o from the .la pieces.
noinst_LTLIBRARIES         = libbirnet.la
libbirnetincludedir        = $(includedir)/birnet
libbirnetinclude_HEADERS   = $(birnet_headers) $(birnet_generated_headers)
libbirnet_la_SOURCES       = $(birnet_sources)
libbirnet_la_DEPENDENCIES  =
libbirnet_la_LIBADD        = $(BIRNET_LIBS) -lm
libbirnet_la_LDFLAGS       = -no-undefined # -Wl,-Bsymbolic
# keep this .o rule in sync with the corresponding .la rule from Makefile.in
libbirnet.o: $(libbirnet_la_OBJECTS) $(libbirnet_la_DEPENDENCIES)
	$(CXXLINK)  $(libbirnet_la_LDFLAGS) $(libbirnet_la_OBJECTS) # $(libbirnet_la_LIBADD) $(LIBS)
all-am: libbirnet.o
CLEANFILES += libbirnet.lo libbirnet.o
EXTRA_DIST += $(birnet_private_headers)

# === birnetconfig.h ===
birnetconfig.h: $(top_builddir)/config.status # Makefile
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* select programming environment */"						>> xgen-$(@F) \
	&& echo "#ifndef _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#define _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* standard C code wrappers */"						>> xgen-$(@F) \
	&& echo "#ifdef  __cplusplus"								>> xgen-$(@F) \
	&& echo '#define BIRNET_EXTERN_C_BEGIN()	extern "C" {'				>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_END()		}"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_BEGIN()"						>> xgen-$(@F) \
	&& echo "#define BIRNET_EXTERN_C_END()"							>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* birnet version */"								>> xgen-$(@F) \
	&& echo "#define BIRNET_MAJOR_VERSION		(@BIRNET_MAJOR_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_MINOR_VERSION		(@BIRNET_MINOR_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_MICRO_VERSION		(@BIRNET_MICRO_VERSION@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_BINARY_AGE		(@BIRNET_BINARY_AGE@)"			>> xgen-$(@F) \
	&& echo "#define BIRNET_INTERFACE_AGE		(@BIRNET_INTERFACE_AGE@)"		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* log domain */"								>> xgen-$(@F) \
	&& echo "#ifndef BIRNET_LOG_DOMAIN"							>> xgen-$(@F) \
	&& echo "#ifdef  G_LOG_DOMAIN"								>> xgen-$(@F) \
	&& echo "#define BIRNET_LOG_DOMAIN  G_LOG_DOMAIN"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define BIRNET_LOG_DOMAIN  ((const char*) 0)"					>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* version checks */"								>> xgen-$(@F) \
	&& echo "#define BIRNET_CHECK_VERSION(major,minor,micro)	\\"			>> xgen-$(@F) \
	&& echo "  (BIRNET_MAJOR_VERSION > (major) || \\"					>> xgen-$(@F) \
	&& echo "   (BIRNET_MAJOR_VERSION == (major) && BIRNET_MINOR_VERSION > (minor)) || \\"	>> xgen-$(@F) \
	&& echo "   (BIRNET_MAJOR_VERSION == (major) && BIRNET_MINOR_VERSION == (minor) && \\"	>> xgen-$(@F) \
	&& echo "    BIRNET_MICRO_VERSION >= (micro)))"						>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* configure results */"							>> xgen-$(@F) \
	&& echo "#define BIRNET_OS_@BIRNET_OS@		(1) /* one of: @BIRNET_OS_CHOICES@ */"	>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_POINTER		(@BIRNET_SIZEOF_POINTER@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_LONG		(@BIRNET_SIZEOF_LONG@)"			>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_SYS_TYPESH_UINT	(@BIRNET_SIZEOF_SYS_TYPESH_UINT@)"	>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_PTH_MUTEX_T	(@BIRNET_SIZEOF_PTH_MUTEX_T@)" 		>> xgen-$(@F) \
	&& echo "#define BIRNET_SIZEOF_PTH_COND_T	(@BIRNET_SIZEOF_PTH_COND_T@)" 		>> xgen-$(@F) \
	&& echo "#define BIRNET_HAVE_MUTEXATTR_SETTYPE	(@BIRNET_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        BIRNET_SIZEOF_PTH_MUTEX_T && \\" 	>> xgen-$(@F) \
	&& echo "                                        BIRNET_SIZEOF_PTH_COND_T)" 		>> xgen-$(@F) \
	&& echo "#define BIRNET_MEMORY_BARRIER_NEEDED   (@BIRNET_MEMORY_BARRIER_NEEDED@)"	>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLIN 		(@BIRNET_SYSVAL_POLLIN@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLPRI		(@BIRNET_SYSVAL_POLLPRI@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLOUT		(@BIRNET_SYSVAL_POLLOUT@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLRDNORM	(@BIRNET_SYSVAL_POLLRDNORM@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLRDBAND 	(@BIRNET_SYSVAL_POLLRDBAND@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLWRNORM   	(@BIRNET_SYSVAL_POLLWRNORM@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLWRBAND	(@BIRNET_SYSVAL_POLLWRBAND@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLERR		(@BIRNET_SYSVAL_POLLERR@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLHUP		(@BIRNET_SYSVAL_POLLHUP@)"		>> xgen-$(@F) \
	&& echo "#define BIRNET_SYSVAL_POLLNVAL		(@BIRNET_SYSVAL_POLLNVAL@)"		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* Generated data ends here */" 						>> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
CLEANFILES += birnetconfig.h
$(libbirnet_la_OBJECTS): birnetconfig.h

noinst_PROGRAMS = $(ALLTESTS)
progs_ldadd     = libbirnet.o $(BIRNET_LIBS) -lm

noinst_PROGRAMS       += birnet-zintern
birnet_zintern_SOURCES = birnet-zintern.cc
birnet_zintern_LDADD   = $(progs_ldadd)
birnet_zintern_DEPS    = $(progs_deps)

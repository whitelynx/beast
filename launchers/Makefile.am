# launchers - program starters
# Copyright (C) 2004 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I. $(LAUNCHER_CFLAGS) -DG_DISABLE_CONST_RETURNS

DEFS += $(strip                         		\
        -DBINDIR=\"$(bindir)\" 				\
)

#
# programs to build
#
bin_PROGRAMS   = beast bsescm
beast_SOURCES  = suidmain.c beaststart.c
bsescm_SOURCES = suidmain.c bseshstart.c
EXTRA_DIST    += suidmain.h

#
# make beast and bsescm wrapper suid to allow renicing
#
install-exec-hook:	# runs after install-binPROGRAMS
	true \
	&& ( chown root $(DESTDIR)$(bindir)/beast $(DESTDIR)$(bindir)/bsescm   \
	&&   chmod 4755 $(DESTDIR)$(bindir)/beast $(DESTDIR)$(bindir)/bsescm ) \
	|| ( echo "*** WARNING *** beast and bsescm need to be installed as root to allow renicing ***" && sleep 5 )

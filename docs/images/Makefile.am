# BEAST - Better Audio System
# Copyright (C) 1998-2006 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = # none

dist_images = $(strip		\
	module-voices.png	\
	beast-components.png	\
	beast-progs.png		\
)

dist_dias = $(strip		\
	beast-components.dia	\
	beast-progs.dia		\
	module-voices.dia	\
)

docimages_DATA = ${dist_images}
docimagesdir = ${beastimagesdir}

EXTRA_DIST += $(dist_images) $(dist_dias)

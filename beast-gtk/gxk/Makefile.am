# GXK - Gtk+ Extension Kit
# Copyright (C) 2002 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS =

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/beast-gtk -I$(top_builddir)/beast-gtk -I$(srcdir) $(GXK_CFLAGS) # -I.. for <gxk/*.h> includes

DEFS += $(strip \
	-DGTK_DISABLE_COMPAT_H 	\
	-DGXK_COMPILATION	\
	-DG_LOG_DOMAIN=\"GXK\" 	\
)
# -DGTK_DISABLE_DEPRECATED -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN

noinst_LIBRARIES = libgxk.a

gxk_headers = $(strip \
	gxkled.h	gxkcanvas.h		gxkdialog.h	gxkauxwidgets.h \
	gxkstatusbar.h	gxkstock.h		gxktexttools.h	gxklistwrapper.h \
	gxkrackeditor.h	gxkutils.h		gxkpolygon.h	gxkcellrendererpopup.h \
	gxkradget.h	gxkaction.h		gxkglobals.h	gxkradgetfactory.h \
	glewidgets.h	gxkracktable.h		gxkrackitem.h	gxksimplelabel.h \
	gxkparam.h	gxkscrollcanvas.h	gxkimagecache.h	gxklogadjustment.h \
	gxkmenubutton.h	gxkspline.h		gxkassortment.h gxknotebook.h \
	gxk.h \
)
gxk_c_sources = $(strip \
	gxkled.c	gxkcanvas.c		gxkdialog.c	gxkauxwidgets.c \
	gxkstatusbar.c	gxkstock.c		gxktexttools.c	gxklistwrapper.c \
	gxkrackeditor.c	gxkutils.c		gxkpolygon.c	gxkcellrendererpopup.c \
	gxkradget.c	gxkaction.c		gxkglobals.c	gxkradgetfactory.c \
	glewidgets.c	gxkracktable.c		gxkrackitem.c	gxksimplelabel.c \
	gxkparam.c	gxkscrollcanvas.c	gxkimagecache.c	gxklogadjustment.c \
	gxkmenubutton.c	gxkspline.c		gxkassortment.c gxknotebook.c \
)
gxk_extra_sources = $(strip \
	gxkparam-entry.c	gxkparam-label.c	\
	gxkparam-scale.c	gxkparam-spinner.c	\
	gxkparam-toggle.c	\
	gxkrackcovers.c		\
	gxkmarshal.list		\
)

EXTRA_DIST += $(gxk_headers) $(gxk_extra_sources)

GLIB_MKENUMS = glib-mkenums

CLEANSTAMPFILES =
if WITH_PERLRULE
CLEANFILES += $(CLEANSTAMPFILES)
else
MAINTAINERCLEANFILES += $(CLEANSTAMPFILES)
endif

# gxkgentypes.h
GENERATED_EXTRA_DIST += gxkgentypes.h
$(libgxk_a_OBJECTS): gxkgentypes.h
gxkgentypes.h: @PERLRULE@ stamp-gxkgentypes.h ;
CLEANSTAMPFILES += stamp-gxkgentypes.h
stamp-gxkgentypes.h: @PERLRULE@ $(gxk_headers)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "#define GXK_TYPE_@ENUMSHORT@\t    (gxk__type_id__@EnumName@)\n" \
	    --eprod "extern GType gxk__type_id__@EnumName@;" \
	      $(gxk_headers) \
	) > xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $(srcdir)/gxkgentypes.h || cp xgen-$(@F) $(srcdir)/gxkgentypes.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $(@F)
# gxkgentypes.c
GENERATED_CLEANFILES += gxkgentypes.c
$(libgxk_a_OBJECTS): gxkgentypes.c
gxkgentypes.c: $(gxk_headers)
	( \
	  cd $(srcdir) \
	  && $(GLIB_MKENUMS) \
	    --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	    --vhead "\nGType gxk__type_id__@EnumName@ = 0;" \
	    --vhead "\nstatic G@Type@Value @enum_name@_values[] = {" \
	    --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	    --vtail "  { 0, NULL, NULL }\n};\n" \
	      $(gxk_headers) \
	  && $(GLIB_MKENUMS) \
	    --fhead "static const GxkTypeGenerated generated_type_entries[] = {" \
	    --fprod "\n/* --- @filename@ --- */" \
	    --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &gxk__type_id__@EnumName@, @enum_name@_values }," \
	    --ftail "\n};" \
	      $(gxk_headers) \
	) > xgen-$(@F) \
        && cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# gxkmarshal.h
GENERATED_CLEANFILES += gxkmarshal.h
$(libgxk_a_OBJECTS): gxkmarshal.h
gxkmarshal.h: gxkmarshal.list
	( \
	  cd $(srcdir) \
	  && glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --header \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# gxkmarshal.c
GENERATED_CLEANFILES += gxkmarshal.c
$(libgxk_a_OBJECTS): gxkmarshal.c
gxkmarshal.c: gxkmarshal.list
	( \
	  cd $(srcdir) \
	  && glib-genmarshal --prefix=gxk_marshal gxkmarshal.list --body \
	) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)

libgxk_a_SOURCES = $(gxk_c_sources)

noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(strip					\
	libgxk.a $(GXK_LIBS) 					\
	$(top_builddir)/sfi/libsfi.o $(SFI_LIBS) 		\
	$(top_builddir)/birnet/libbirnet.o $(BIRNET_LIBS)	\
)


noinst_PROGRAMS += gxktest
gxktest_SOURCES  = gxktest.c dummy.cc
gxktest_LDADD    = $(progs_LDADD)

noinst_PROGRAMS   += splinetest
splinetest_SOURCES = splinetest.c dummy.cc
splinetest_LDADD   = $(progs_LDADD)


#
# convenience targets for generated source files
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) $(CLEANSTAMPFILES)
generated: clean-generated $(GENERATED)

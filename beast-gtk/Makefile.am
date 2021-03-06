# BEAST - Better Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = gxk dialogs icons

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(BEAST_CFLAGS) -DG_DISABLE_CONST_RETURNS -DGTK_ENABLE_BROKEN
# -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_COMPAT_H

# we specify the logging domain here
DEFS += -DG_LOG_DOMAIN="\"BEAST\""

# special profiling hooks
PROFILE_SOURCE = # profile.c
DEFS += # $(subst profile.c, -pg -a, $(findstring profile.c, $(<F)))
LIBS += # -lefence # -pg 

#
# setup source file variables
#
# BEAST header files that don't get installed
beast_headers = $(strip \
	bsttrackrollctrl.h	bstxframe.h	bstscrollgraph.h 	\
	bstdbmeter.h		bstbusmixer.h	bstbuseditor.h		bstitemseqdialog.h  \
	bstcanvassource.h	bstapp.h	bstasciipixbuf.h	bstcanvaslink.h	    \
	bstpatterncolumns.h	bstxkb.h	bstpatternview.h	bstpatternctrl.h    \
	bstkeybindings.h	bstprofiler.h	bstgrowbar.h		bstbusview.h        \
	bstpianorollctrl.h	bstpartview.h	bstpianoroll.h		bstplayback.h	    \
	bsttrackroll.h		bstcluehunter.h	bstprojectctrl.h	bstcxxutils.h	    \
	bstauxdialogs.h		bstsegment.h	bsteventrollctrl.h	bsteventroll.h	    \
	bstsnifferscope.h	bstwaveview.h	bstfiledialog.h		bstgconfig.h	    \
	bstlogadjustment.h	bstitemview.h	bstservermonitor.h	bstknob.h	    \
	bstparamview.h		bstmenus.h	bstparam.h		bstpartdialog.h	    \
	bstprocbrowser.h	bstqsampler.h	bstpreferences.h	bstprocedure.h	    \
	bstrackeditor.h		bstrackitem.h	bstracktable.h		bstsequence.h	    \
	bstsnetrouter.h		bstsplash.h	bsttrackview.h		bstsupershell.h	    \
	bstusermessage.h	bstdial.h	bsttracksynthdialog.h	bstwaveeditor.h	    \
	bstzoomedwindow.h	bstskinconfig.h	bstmsgabsorb.h		bstsampleeditor.h   \
	bstrackview.h		bsttreestores.h	bstbseutils.h		bstutils.h	    \
	bstdefs.h \
)
EXTRA_DIST += $(beast_headers)
# BEAST sources to build the program from
beast_sources = $(strip \
	bsttrackrollctrl.c	bstxframe.c	bstscrollgraph.c	\
	bstdbmeter.c		bstbusmixer.c	bstbuseditor.c		bstitemseqdialog.c  \
	bstcanvassource.c	bstapp.c	bstasciipixbuf.c	bstcanvaslink.c	    \
	bstpatterncolumns.c	bstxkb.c	bstpatternview.c	bstpatternctrl.c    \
	bstkeybindings.c	bstprofiler.c	bstgrowbar.c		bstbusview.c        \
	bstpianorollctrl.c	bstpartview.c	bstpianoroll.c		bstplayback.c	    \
	bsttrackroll.c		bstcluehunter.c	bstprojectctrl.c	bstcxxutils.cc	    \
	bstauxdialogs.c		bstsegment.c	bsteventrollctrl.c	bsteventroll.c	    \
	bstsnifferscope.c	bstwaveview.c	bstfiledialog.c		bstgconfig.c	    \
	bstlogadjustment.c	bstitemview.c	bstservermonitor.c	bstknob.c	    \
	bstparamview.c		bstmenus.c	bstparam.c		bstpartdialog.c	    \
	bstprocbrowser.c	bstqsampler.c	bstpreferences.c	bstprocedure.c	    \
	bstrackeditor.c		bstrackitem.c	bstracktable.c		bstsequence.c	    \
	bstsnetrouter.c		bstsplash.c	bsttrackview.c		bstsupershell.c	    \
	bstusermessage.c	bstdial.c	bsttracksynthdialog.c	bstwaveeditor.c	    \
	bstzoomedwindow.c	bstskinconfig.c	bstmsgabsorb.c		bstsampleeditor.c   \
	bstrackview.c		bsttreestores.c	bstbseutils.c		bstutils.c	    \
	$(PROFILE_SOURCE) \
)
# BEAST sources that get included (don't have own .lo rules)
beast_extra_files = $(strip 						\
	bstparam-automation.c						\
	bstparam-choice.c		bstparam-note-sequence.c	\
	bstparam-note-spinner.c		bstparam-proxy.c		\
	bstparam-color-spinner.c	bstparam-scale.c		\
	bstparam-searchpath.c		bstparam-time.c			\
	bstparam-item-seq.c		bstrackeditor-covers.c		\
	\
	bstmarshal.list			bstrecords.idl			\
)
EXTRA_DIST += $(beast_extra_files)

GLIB_MKENUMS = glib-mkenums
SFIDL = ../sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)

#
# rules to generate built sources
#
# bstgentypes.h
GENERATED_CLEANFILES += bstgentypes.h
CLEANFILES += stamp-bstgentypes.h
$(beast_vOBJECTS): bstgentypes.h
bstgentypes.h: stamp-bstgentypes.h ;
stamp-bstgentypes.h: $(beast_headers) $(srcdir)/bstrecords.idl $(SFIDL)
	cd .	\
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "#define BST_TYPE_@ENUMSHORT@\t    (bst__type_id__@EnumName@)\n" \
	  --eprod "extern GType bst__type_id__@EnumName@;" \
	    $(beast_headers) ) > xgen-$(@F) \
	&& $(SFIDL) --host-c --header ${srcdir}/bstrecords.idl >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) bstgentypes.h || cp xgen-$(@F) bstgentypes.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $@
# bstgentypes.c
GENERATED_CLEANFILES += bstgentypes.c
$(beast_vOBJECTS): bstgentypes.c
bstgentypes.c: bstgentypes.h $(beast_headers) ${srcdir}/bstrecords.idl $(SFIDL)
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --eprod "\nGType bst__type_id__@EnumName@ = 0;" \
	    $(beast_headers) ) > xgen-$(@F) \
	&& $(SFIDL) --host-c --source --init _bst_init_idl ${srcdir}/bstrecords.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstenum_arrays.c
GENERATED_CLEANFILES += bstenum_arrays.c
$(beast_vOBJECTS): bstenum_arrays.c
bstenum_arrays.c: $(beast_headers)
	cd . \
	&& ( cd ${srcdir} && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static G@Type@Value @enum_name@_values[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	  --vtail "  { 0, NULL, NULL }\n};\n" \
	    $(beast_headers) ) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstenum_list.c
GENERATED_CLEANFILES += bstenum_list.c
$(beast_vOBJECTS): bstenum_list.c
bstenum_list.c: $(beast_headers)
	cd . \
	&& ( cd $(srcdir) && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */" \
	  --eprod "  { \"@EnumName@\", G_TYPE_@TYPE@, &bst__type_id__@EnumName@, @enum_name@_values }," \
	    $(beast_headers) ) > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstgenbseapi.h
GENERATED_CLEANFILES += bstgenbseapi.h
CLEANFILES += stamp-bstgenbseapi.h
$(beast_vOBJECTS): bstgenbseapi.h
bstgenbseapi.h: stamp-bstgenbseapi.h ;
stamp-bstgenbseapi.h: $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd . \
	&& $(SFIDL) $(SFIDL_INC) --client-c --header --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) bstgenbseapi.h || cp xgen-$(@F) bstgenbseapi.h) \
	&& rm -f xgen-$(@F) \
	&& echo timestamp > $@
# bstgenbseapi.c
GENERATED_CLEANFILES += bstgenbseapi.c
$(beast_vOBJECTS): bstgenbseapi.c
bstgenbseapi.c: bstgenbseapi.h $(top_srcdir)/bse/*.idl $(top_builddir)/bse/*.idl $(SFIDL)
	cd .	\
	&& echo -e "/* #include \"bstgenbseapi.h\" */\n" > xgen-$(@F) \
	&& $(SFIDL) $(SFIDL_INC) --client-c --source --prefix beast_ $(top_srcdir)/bse/bse.idl >> xgen-$(@F) \
	&& cp xgen-$(@F) bstgenbseapi.c \
	&& rm -f xgen-$(@F)
# bstmarshal.h
GENERATED_CLEANFILES += bstmarshal.h
$(beast_vOBJECTS): bstmarshal.h
bstmarshal.h: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --header > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
# bstmarshal.c
GENERATED_CLEANFILES += bstmarshal.c
$(srcdir)/bstutils.c: bstmarshal.c
bstmarshal.c: bstmarshal.list
	glib-genmarshal --prefix=bst_marshal $(srcdir)/bstmarshal.list --body > xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)


#
# convenience targets for generated source files  
#
.PHONY: generated clean-generated
clean-generated: clean
	rm -f $(GENERATED) stamp-*
generated: clean-generated $(GENERATED)


#
# build rules for programs
#
bin_PROGRAMS = beast-$(BIN_VERSION)
beast___BIN_VERSION__SOURCES = bstmain.c $(beast_sources)
beast___BIN_VERSION__LDADD = $(progs_LDADD)
beast_vOBJECTS  = $(beast___BIN_VERSION__OBJECTS)
noinst_PROGRAMS = $(ALLTESTS)
progs_LDADD     = $(strip                                       \
	$(BEAST_LIBS) 						\
	./gxk/libgxk.a $(GXK_LIBS)                              \
	$(top_builddir)/bse/libbse.la $(BSE_LIBS)               \
)

noinst_PROGRAMS += tsmview
tsmview_SOURCES  = tsmview.c cxxdummy.cc
tsmview_LDADD    = $(progs_LDADD)

noinst_PROGRAMS += testgui
testgui_SOURCES  = testgui.c $(beast_sources)
testgui_LDADD    = $(progs_LDADD)

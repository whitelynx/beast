# BEAST - Bedevilled Audio System
# Copyright (C) 1998-2002 Tim Janik
#
## Makefile.am for generated documentation

SUBDIRS = # none

docsdir = ${beastdocsdir}
docs_DATA = ${markup_targets}

# install man pages
man_MANS = sfi.3 bse.3 beast-gxk.3 bse-procs.3 bse-structs.3
# installed docs
markup_targets = $(strip	\
	faq.markup		\
	release-notes.markup	\
	news.markup		\
	gsl-mplan.markup	\
	quickstart.markup	\
	beast-index.markup	\
	sfi.markup		\
	bse.markup		\
	beast-gxk.markup	\
	bse-procs.markup	\
	bse-structs.markup	\
)
# website html generation
html_targets = $(patsubst %.markup, %.html, $(markup_targets)) # $(patsubst %.3, %.html, $(man_MANS))
# extra dist
build_tools = $(strip		\
	man.xsl			\
	html.xsl		\
	markup.xsl		\
	markup.dtd		\
	texinfo.dtd		\
	scandocs.pl		\
)
EXTRA_DIST += ${build_tools}

#
# rules
#
html-targets:	$(html_targets)
lint:
	for i in $(markup_targets) ; do \
		xmllint --noout --loaddtd --valid $$i ; \
	done
.PHONY: html-targets lint

#
# reference documentation
#
sfi_refsrc_globs=$(top_srcdir)/sfi/sfi*.[hc]
bse_refsrc_globs=$(top_srcdir)/bse/bse*.[hc] $(top_srcdir)/bse/gsl*.[hc]
gxk_refsrc_globs=$(top_srcdir)/beast-gtk/gxk/gxk*.[hc]
AUTODOC=$(top_builddir)/bse/bseautodoc
SCANDOCS=$(srcdir)/scandocs.pl

sfi.texi: $(sfi_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "SFI-Functions" \
		    --blurb "SFI Function Reference" \
		    --package "BEAST-$(VERSION)" \
		      $(sfi_refsrc_globs) >$@
bse.texi: $(bse_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "BSE-Functions" \
		    --blurb "BSE Function Reference" \
		    --package "BEAST-$(VERSION)" \
		      $(bse_refsrc_globs) >$@
beast-gxk.texi: $(gxk_refsrc_globs) $(SCANDOCS) $(top_srcdir)/configure
	$(SCANDOCS) --name "GXK-Functions" \
		    --blurb "Gtk+ Extension Kit Function Reference" \
		    --package "BEAST-$(VERSION)" \
		      $(gxk_refsrc_globs) >$@
bse-procs.texi: $(AUTODOC) $(top_srcdir)/configure
	$(AUTODOC) procs >$@
bse-structs.texi: $(AUTODOC) $(top_srcdir)/configure
	$(AUTODOC) structs >$@
temp_texi = sfi.texi bse.texi beast-gxk.texi bse-procs.texi bse-structs.texi

#
# texi/xml/target rules
#
XMLANTISPACE=$(top_builddir)/docs/utils/xmlantispace
if HAVE_XMLMAKEINFO_AND_XSLTPROC
%.xml: $(srcdir)/../%.texi	$(XMLANTISPACE)
	$(XMLMAKEINFO) -I $(srcdir)/.. --xml $< -o - | $(XMLANTISPACE) >$@
%.xml: %.texi			$(XMLANTISPACE)
	$(XMLMAKEINFO) -I $(srcdir)/.. --xml $< -o - | $(XMLANTISPACE) >$@
%.markup: %.xml			$(srcdir)/markup.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --stringparam revision "`date -r $<`" $(srcdir)/markup.xsl $< >$@
%.html: %.xml			$(srcdir)/html.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --stringparam revision "`date -r $<`" $(srcdir)/html.xsl $< >$@
%.3: %.xml			$(srcdir)/man.xsl $(srcdir)/texinfo.dtd
	$(XSLTPROC) --stringparam revision "`date -r $<`" \
		    --param man_section 3                 $(srcdir)/man.xsl $< >$@
gsl-mplan.xml: $(top_srcdir)/bse/gsl-mplan.txt
	cd . \
	&& echo '<texinfo>' > $@ \
	&& echo '<settitle>GSL Flow Engine</settitle><preformat><para>' >> $@ \
	&& sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' < $< >> $@ \
	&& echo '</para></preformat></texinfo>' >> $@
news.xml: $(top_srcdir)/NEWS
	cd . \
	&& echo '<texinfo>' > $@ \
	&& echo '<settitle>BEAST/BSE NEWS</settitle><preformat><para>' >> $@ \
	&& sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' < $< >> $@ \
	&& echo '</para></preformat></texinfo>' >> $@
conditional_clean = $(markup_targets) gsl-mplan.xml news.xml faq.xml $(man_MANS) $(temp_texi)
endif

CLEANFILES += $(conditional_clean) $(html_targets)
MAINTAINERCLEANFILES +=

FEATURE_TESTS = bseadder.test
CLEANFILES = *.test.current *.bse.wav xgen-bp beastconf.sh

noinst_build_sources += beastconf.sh
beastconf.sh:
	cd . \
	&& echo "# Generated data from $< (by make $@)" > xgen-bp \
	&& echo >> xgen-bp \
	&& echo "BSE_VERSION=$(BSE_VERSION)" >> xgen-bp \
	&& echo "top_builddir=$(top_builddir)" >> xgen-bp \
	&& chmod 755 xgen-bp \
	&& (cp xgen-bp $@) \
	&& rm -f xgen-bp

all: beastconf.sh

test: beastconf.sh
	featuretest.sh $(FEATURE_TESTS)
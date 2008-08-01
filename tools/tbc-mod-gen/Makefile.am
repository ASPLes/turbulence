EXTRA_DIST = tbc-mod-gen.dtd tbc-mod-gen.dtd.h

INCLUDES = -DCOMPILATION_DATE=`date +%s` -I ../../src \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" -DAXL_VERSION=\"$(AXL_VERSION)\" \
	   -DVORTEX_VERSION=\"$(VORTEX_VERSION)\" \
	   $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS)\
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS        = tbc-mod-gen
tbc_mod_gen_SOURCES = main.c support.c support.h
tbc_mod_gen_LDADD   = $(AXL_LIBS) $(EXARG_LIBS) ../../src/libturbulence.la

tbcmodgendir   = $(datadir)/turbulence
tbcmodgen_DATA = tbc-mod-gen.dtd


tbc-mod-gen.dtd.h: tbc-mod-gen.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer


INCLUDES = -DCOMPILATION_DATE=`date +%s` -I ../../src \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS)\
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS        = tbc-mod-gen
tbc_mod_gen_SOURCES = main.c support.c support.h
tbc_mod_gen_LDADD   = $(AXL_LIBS) $(EXARG_LIBS) ../../src/libturbulence.la





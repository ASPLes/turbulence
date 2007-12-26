INCLUDES = -DCOMPILATION_DATE=`date +%s` -I ../../src \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS)\
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS           = tbc-dblist-mgr
tbc_dblist_mgr_SOURCES = tbc-dblist-mgr.c
tbc_dblist_mgr_LDADD   = $(AXL_LIBS) $(EXARG_LIBS) ../../src/libturbulence.la







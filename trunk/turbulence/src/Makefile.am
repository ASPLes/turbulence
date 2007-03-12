EXTRA_DIST = 

INCLUDES = -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS = turbulence

turbulence_SOURCES = \
	main.c \
	turbulence.c turbulence.h \
	turbulence-config.c turbulence.h

turbulence_LDADD = $(AXL_LIBS) $(VORTEX_LIBS) $(EXARG_LIBS)

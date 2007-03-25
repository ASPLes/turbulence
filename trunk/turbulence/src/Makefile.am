EXTRA_DIST = 

INCLUDES = -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS = turbulence

turbulence_SOURCES = \
	main.c \
	turbulence-moddef.h \
	turbulence.c turbulence.h \
	turbulence-config.c turbulence.h \
	turbulence-run.c turbulence-run.h \
	turbulence-sasl.c turbulence-sasl.h \
	turbulence-module.c turbulence-module.h \
	turbulence-log.c turbulence-log.h

turbulence_LDFLAGS = -Wl,-export-dynamic -ldl

turbulence_LDADD = $(AXL_LIBS) $(VORTEX_LIBS) $(EXARG_LIBS) 

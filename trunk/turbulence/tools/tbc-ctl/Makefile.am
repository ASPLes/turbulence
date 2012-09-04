INCLUDES = -DCOMPILATION_DATE=`date +%s` -I ../../src \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) -I$(READLINE_PATH)/include \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

EXTRA_DIST = tbc-setup-mod-radmin.py

bin_PROGRAMS    = turbulence-ctl
bin_SCRIPTS     = tbc-setup-mod-radmin.py
turbulence_ctl_SOURCES = tbc-ctl.c
turbulence_ctl_LDADD   = $(AXL_LIBS) $(EXARG_LIBS) $(VORTEX_SASL_LIBS) -L$(READLINE_PATH)/lib -lreadline ../../src/libturbulence.la







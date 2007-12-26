EXTRA_DIST = mod-test.xml 

INCLUDES = -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-test.la
mod_test_la_SOURCES  = mod-test.c
mod_test_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
modconfdir   = $(sysconfdir)/turbulence/mods-available
modconf_DATA = mod-test.xml mod-test.win32.xml

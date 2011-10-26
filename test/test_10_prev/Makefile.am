EXTRA_DIST = mod_test_10_prev.xml mod_test_10_prev.win32.xml
INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_test_10_prev.la
mod_test_10_prev_la_SOURCES  = mod_test_10_prev.c 
mod_test_10_prev_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = `../../turbulence-config --mod-dir`

# configure site module installation
modconfdir   = `../../turbulence-config --mod-xml`
modconf_DATA = mod_test_10_prev.xml mod_test_10_prev.win32.xml



# Module definition
EXTRA_DIST = mod-test-11.xml

INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_test_11.la
mod_test_11_la_SOURCES  = mod_test_11.c 
mod_test_11_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = `turbulence-config --mod-dir`

# configure site module installation
modconfdir   = `turbulence-config --mod-xml`
modconf_DATA = mod_test_11.xml mod_test_11.win32.xml

mod_test_11.xml mod_test_11.win32.xml:
	echo "<mod-turbulence location=\"`turbulence-config --mod-dir`/mod_test_11.so\"/>" > mod_test_11.xml
	echo "<mod-turbulence location=\"../modules/mod_test_11.dll\"/>" > mod_test_11.win32.xml

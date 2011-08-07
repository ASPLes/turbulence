# Module definition
EXTRA_DIST = mod-test-10b.xml

INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_test_10b.la
mod_test_10b_la_SOURCES  = mod_test_10b.c 
mod_test_10b_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = `../../turbulence-config --mod-dir`

# configure site module installation
modconfdir   = `../../turbulence-config --mod-xml`
modconf_DATA = mod_test_10b.xml mod_test_10b.win32.xml

mod_test_10b.xml mod_test_10b.win32.xml:
	echo "<mod-turbulence location=\"`find ../test_10b_module/ -name *.so`\"/>" > mod_test_10b.xml
	echo "<mod-turbulence location=\"`find ../test_10b_module/ -name *.dll`\"/>" > mod_test_10b.win32.xml

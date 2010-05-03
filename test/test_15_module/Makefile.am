# Module definition
EXTRA_DIST = 

INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_test_15.la
mod_test_15_la_SOURCES  = mod_test_15.c 
mod_test_15_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = `../../turbulence-config --mod-dir`

# configure site module installation
modconfdir   = `../../turbulence-config --mod-xml`
modconf_DATA = mod_test_15.xml mod_test_15.win32.xml

mod_test_15.xml mod_test_15.win32.xml:
	echo "<mod-turbulence location=\"`find ../test_15_module/ -name *.so`\"/>" > mod_test_15.xml
	echo "<mod-turbulence location=\"`find ../test_15_module/ -name *.dll`\"/>" > mod_test_15.win32.xml

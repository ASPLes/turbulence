# Module definition
EXTRA_DIST = mod-radmin-def.xml mod_radmin.win32.xml

INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_radmin.la
mod_radmin_la_SOURCES  = mod_radmin.c  mod_radmin.h
mod_radmin_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = `turbulence-config --mod-dir`

# configure site module installation
modconfdir   = `turbulence-config --mod-xml`
modconf_DATA = mod_radmin.xml

mod_radmin.xml mod_radmin.win32.xml:
	echo "<mod-turbulence location=\"`turbulence-config --mod-dir`/mod_radmin.so\"/>" > mod_radmin.xml
	echo "<mod-turbulence location=\"../modules/mod_radmin.dll\"/>" > mod_radmin.win32.xml

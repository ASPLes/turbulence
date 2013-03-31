# Module definition
EXTRA_DIST = template.xml

INCLUDES = -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) $(VORTEX_WEBSOCKET_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_websocket.la
mod_websocket_la_SOURCES  = mod_websocket.c 
mod_websocket_la_LDFLAGS  = -module -ldl $(VORTEX_WEBSOCKET_LIBS)

# reconfigure module installation directory
libdir = `turbulence-config --mod-dir`

# configure site module configuration installation
etcdir   = $(sysconfdir)/turbulence/websocket
etc_DATA = websocket.example.conf

# configure site module installation
modconfdir   = $(sysconfdir)/turbulence/mods-available
modconf_DATA = mod_websocket.xml mod_websocket.win32.xml

mod_websocket.xml mod_websocket.win32.xml:
	echo "<mod-turbulence location=\"`turbulence-config --mod-dir`/mod_websocket.so\"/>" > mod_websocket.xml
	echo "<mod-turbulence location=\"../modules/mod_websocket.dll\"/>" > mod_websocket.win32.xml

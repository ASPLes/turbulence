EXTRA_DIST = mod-tunnel.xml tunnel.conf resolver.xml

INCLUDES = -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) $(VORTEX_TUNNEL_CFLAGS)

lib_LTLIBRARIES      = mod-tunnel.la
mod_tunnel_la_SOURCES  = mod-tunnel.c mod-tunnel.h
mod_tunnel_la_LDFLAGS  = -module -ldl $(VORTEX_TUNNEL_LIBS)

# reconfigure module installation directory
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod-tunnel.xml mod-tunnel.win32.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/tunnel
modconf_DATA = tunnel.conf resolver.xml

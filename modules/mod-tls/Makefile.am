# Module definition
EXTRA_DIST = mod_tls.xml mod_tls.win32.xml tls.example.conf

INCLUDES = $(compiler_options) -Wall -g -ansi $(TURBULENCE_CFLAGS) -I../../src -DCOMPILATION_DATE=`date +%s` \
	-DVERSION=\"$(VERSION)\" \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) $(VORTEX_TLS_CFLAGS)

# configure module binary
lib_LTLIBRARIES      = mod_tls.la
mod_tls_la_SOURCES  = mod_tls.c 
mod_tls_la_LDFLAGS  = -module -ldl $(VORTEX_TLS_LIBS)

# reconfigure module installation directory
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod_tls.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/tls
modconf_DATA = tls.example.conf tls.example.conf.tmp

mod_tls.xml mod_tls.win32.xml:
	echo "<mod-turbulence location=\"`turbulence-config --mod-dir`/mod_tls.so\"/>" > mod_tls.xml
	echo "<mod-turbulence location=\"../modules/mod_tls.dll\"/>" > mod_tls.win32.xml

# render html version from default configuration files
%.conf.tmp: %.conf
	axl-knife -i $< -o $@ -e -p fragment
EXTRA_DIST = mod-sasl.xml

INCLUDES = -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES   = mod-sasl.la
mod_sasl_la_SOURCES  = mod-sasl.c common-sasl.c common-sasl.h
mod_sasl_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod-sasl.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/sasl
modconf_DATA = sasl.conf auth-db.xml

# tool to admin sasl db 
bin_PROGRAMS = tbc-sasl-conf

tbc_sasl_conf_SOURCES = tbc-sasl-conf.c
tbc_sasl_conf_LDFLAGS = $(EXARG_LIBS) $(AXL_LIBS) ../../src/libturbulence.la 
tbc_sasl_conf_LDADD   = common-sasl.o

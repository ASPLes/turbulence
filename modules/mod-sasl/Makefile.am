EXTRA_DIST = mod-sasl.xml sasl.conf auth-db.xml 

INCLUDES = -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)

sasl_radmin_files = service_dispatch.h service_dispatch.c \
	sasl_radmin_get_users.h sasl_radmin_get_users.c \
	sasl_radmin_struct_sasluser_xml_rpc.h sasl_radmin_struct_sasluser_xml_rpc.c \
	sasl_radmin_array_sasluserarray_xml_rpc.h sasl_radmin_array_sasluserarray_xml_rpc.c \
	sasl_radmin_operate_sasl_user_string_string_bool_bool_int.c sasl_radmin_operate_sasl_user_string_string_bool_bool_int.h \
	sasl_radmin_types.h

lib_LTLIBRARIES   = mod-sasl.la
mod_sasl_la_SOURCES  = mod-sasl.c mod-sasl.h common-sasl.c common-sasl.h $(sasl_radmin_files)
mod_sasl_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
oldlibdir = $(libdir)
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod-sasl.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/sasl
modconf_DATA = sasl.conf auth-db.xml

# tool to admin sasl db 
if ENABLE_TERMIOS
bin_PROGRAMS = tbc-sasl-conf

tbc_sasl_conf_SOURCES = tbc-sasl-conf.c
tbc_sasl_conf_LDFLAGS = $(EXARG_LIBS) $(AXL_LIBS) ../../src/libturbulence.la 
tbc_sasl_conf_LDADD   = common-sasl.o
endif

update-radmin: 
	xml-rpc-gen sasl-radmin.idl --out-server-dir . --only-server --disable-main-file --disable-autoconf

# additional files produced by xml-rpc-gen 
sasl-radmin.idl: 
	xml-rpc-gen sasl-radmin.idl --out-server-dir . --only-server --disable-main-file --disable-autoconf

pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = sasl-radmin.pc

saslradmindir   = $(datadir)/mod-sasl/radmin
saslradmin_DATA = sasl-radmin.idl
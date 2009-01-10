sasl_radmin_files = service_dispatch.h service_dispatch.c \
	sasl_radmin_get_users.h sasl_radmin_get_users.c \
	sasl_radmin_struct_sasluser_xml_rpc.h sasl_radmin_struct_sasluser_xml_rpc.c \
	sasl_radmin_array_sasluserarray_xml_rpc.h sasl_radmin_array_sasluserarray_xml_rpc.c \
	sasl_radmin_operate_sasl_user_string_string_bool_bool_int.c \
	sasl_radmin_operate_sasl_user_string_string_bool_bool_int.h \
	sasl_radmin_types.h

EXTRA_DIST = mod-sasl.xml common.sasl.dtd common.sasl.dtd.h  mod-sasl.win32.xml

INCLUDES = -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) $(VORTEX_XML_RPC_CFLAGS) $(VORTEX_SASL_CFLAGS)

lib_LTLIBRARIES   = mod-sasl.la
mod_sasl_la_SOURCES  = mod-sasl.c mod-sasl.h common-sasl.c common-sasl.h common.sasl.dtd.h $(sasl_radmin_files)
mod_sasl_la_LDFLAGS  = -module -ldl $(VORTEX_XML_RPC_LIBS) $(VORTEX_SASL_LIBS) $(VORTEX_TLS_LIBS)

# reconfigure module installation directory
oldlibdir = $(libdir)
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod-sasl.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/sasl
modconf_DATA = sasl.example.conf auth-db.example.xml remote-admins.example.xml

# tool to admin sasl db 
if ENABLE_TERMIOS
bin_PROGRAMS = tbc-sasl-conf

tbc_sasl_conf_SOURCES = tbc-sasl-conf.c
tbc_sasl_conf_LDFLAGS = $(EXARG_LIBS) $(AXL_LIBS) $(VORTEX_SASL_LIBS) $(VORTEX_TLS_LIBS) ../../src/libturbulence.la 
tbc_sasl_conf_LDADD   = common-sasl.o
endif

update-radmin: 
	xml-rpc-gen-1.1 sasl-radmin.idl --out-server-dir . --only-server --disable-main-file --disable-autoconf

# additional files produced by xml-rpc-gen 
sasl-radmin.idl: 
	xml-rpc-gen-1.1 sasl-radmin.idl --out-server-dir . --only-server --disable-main-file --disable-autoconf

# build sasl-dtd.h file
common.sasl.dtd.h: common.sasl.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer


pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = sasl-radmin.pc

saslradmindir   = $(datadir)/mod-sasl/radmin
saslradmin_DATA = sasl-radmin.idl

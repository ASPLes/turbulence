sasl_radmin_files = service_dispatch.h service_dispatch.c \
	sasl_radmin_get_users.h sasl_radmin_get_users.c \
	sasl_radmin_struct_sasluser_xml_rpc.h sasl_radmin_struct_sasluser_xml_rpc.c \
	sasl_radmin_array_sasluserarray_xml_rpc.h sasl_radmin_array_sasluserarray_xml_rpc.c \
	sasl_radmin_operate_sasl_user_string_string_bool_bool_int.c \
	sasl_radmin_operate_sasl_user_string_string_bool_bool_int.h \
	sasl_radmin_types.h

EXTRA_DIST = mod-sasl.xml common.sasl.dtd common.sasl.dtd.h mysql.sasl.dtd mysql.sasl.dtd.h \
             mod-sasl.win32.xml $(sasl_radmin_files) sasl.example.conf \
	     auth-db.example.xml remote-admins.example.xml gen-mod-sasl-pass.py create-mysql-sasl-database.sql \
	     extension.example.modules auth-db.mysql.example.xml sasl-radmin.idl

INCLUDES = $(compiler_options) -Wall -g -ansi -I ../../src -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) $(VORTEX_XML_RPC_CFLAGS) $(VORTEX_SASL_CFLAGS) $(MYSQL_CFLAGS)

if ENABLE_MYSQL_SUPPORT
BUILD_MOD_SASL_MYSQL = mod-sasl-mysql.la
endif
lib_LTLIBRARIES   = mod-sasl.la $(BUILD_MOD_SASL_MYSQL)

# mod-sasl.so
mod_sasl_la_SOURCES  = mod-sasl.c mod-sasl.h common-sasl.c common-sasl.h common.sasl.dtd.h $(sasl_radmin_files)
mod_sasl_la_LDFLAGS  = -module -ldl $(VORTEX_XML_RPC_LIBS) $(VORTEX_SASL_LIBS) $(VORTEX_TLS_LIBS)

# mod-sasl-mysql.so
if ENABLE_MYSQL_SUPPORT
mod_sasl_mysql_la_SOURCES   = mod_sasl_mysql.c common-sasl.c common-sasl.h common.sasl.dtd.h mysql.sasl.dtd.h
mod_sasl_mysql_la_LDFLAGS   = -module -ldl $(MYSQL_LIBS)
endif

# reconfigure module installation directory
oldlibdir = $(libdir)
libdir = $(prefix)/lib/turbulence/modules

# configure site module installation
moddir   = $(sysconfdir)/turbulence/mods-available
mod_DATA = mod-sasl.xml

# configure site module configuration installation
modconfdir   = $(sysconfdir)/turbulence/sasl
modconf_DATA = sasl.example.conf auth-db.example.xml remote-admins.example.xml extension.example.modules 

# render html version from default configuration files
%.conf.tmp: %.conf
	axl-knife -i $< -o $@ -e -p fragment

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

# build common.sasl.dtd.h file
common.sasl.dtd.h: common.sasl.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer

# build mysql.sasl.dtd.h file
mysql.sasl.dtd.h: mysql.sasl.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer


pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = sasl-radmin.pc

saslradmindir   = $(datadir)/mod-sasl/radmin
saslradmin_DATA = sasl-radmin.idl

# local tasks
all-local: extension.example.modules.xml auth-db.mysql.example.xml-tmp sasl.example.conf.tmp

extension.example.modules.xml: extension.example.modules
	axl-knife -i $< -o $@ -e -p fragment

auth-db.mysql.example.xml-tmp: auth-db.mysql.example.xml
	axl-knife -i $< -o $@ -e -p fragment
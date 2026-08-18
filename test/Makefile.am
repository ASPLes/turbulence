SUBDIRS = test_06a test_10_prev test_10b_module test_11_module test_12_module test_15_module

INCLUDES = -I../src/ -I../modules/mod-sasl/ \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -Wall -ansi  \
	-DAXL_VERSION=\"$(AXL_VERSION)\" -DVORTEX_VERSION=\"$(VORTEX_VERSION)\" $(VORTEX_WEBSOCKET_CFLAGS) \
        -DVERSION=\"$(VERSION)\" 

configuration_files = \
	test_01-b.conf \
	test_06a.conf \
	test06a.ports.conf \
	test06a.listener.conf \
	test_07.conf  \
	test_08.conf  \
	test_08b.conf  \
	test_09.conf  \
	test_10.conf  \
	test_10-a.conf \
	test_10b.conf \
	test_10c.conf \
	test_10d.conf \
	test_10e.conf \
	test_10e-log.conf \
	test_11.conf  \
	test_12.conf  \
	test_12a.conf \
	test_13.conf  \
	test_13a.conf \
	test_13b.conf \
	test_14.conf  \
	test_15.conf  \
	test_16.conf  \
	test_17.conf  \
	test_18.conf  \
	test_19.conf  \
	test_20.conf  \
	test_22.conf  \
	test_23.conf  \
	test_25.conf  \
	test_27.conf



EXTRA_DIST = $(configuration_files) test_05_a_passwd test_12b.sql

# log files produced by test_10e-log.conf: that test runs with log reporting
# enabled on purpose (it is the only one that does) to exercise the log pipe
# branch of turbulence_process_create_child
CLEANFILES = test_10e-log-main.log test_10e-log-error.log test_10e-log-access.log test_10e-log-vortex.log

# replace with bin_PROGRAMS to check performance
noinst_PROGRAMS = test_01  test-websocket-client

test_01_SOURCES = test_01.c
# mod_sasl_mysql_conf.o provides the configuration layer of the mysql sasl
# backend, exercised by test_12d/test_12e. It depends on libaxl alone, so it
# links here without pulling MySQL into the test binary.
test_01_LDADD   = ../src/libturbulence.la ../modules/mod-sasl/common-sasl.o ../modules/mod-sasl/mod_sasl_mysql_conf.o -lcrypt $(AXL_LIBS) $(VORTEX_LIBS) $(VORTEX_SASL_LIBS) \
	$(VORTEX_XML_RPC_LIBS) $(EXARG_LIBS) $(VORTEX_TLS_LIBS) $(VORTEX_WEBSOCKET_LIBS)

test_websocket_client_SOURCES = test-websocket-client.c
test_websocket_client_LDADD   = ../src/libturbulence.la ../modules/mod-sasl/common-sasl.o -lcrypt $(AXL_LIBS) $(VORTEX_LIBS) $(VORTEX_SASL_LIBS) \
	$(VORTEX_XML_RPC_LIBS) $(EXARG_LIBS) $(VORTEX_TLS_LIBS) $(VORTEX_WEBSOCKET_LIBS)

leak-check:
	libtool --mode=execute valgrind --leak-check=yes ./test_01


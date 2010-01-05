INCLUDES = -I../src/ -I../modules/mod-sasl/ \
	$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -Wall -ansi  \
	-DAXL_VERSION=\"$(AXL_VERSION)\" -DVORTEX_VERSION=\"$(VORTEX_VERSION)\" \
        -DVERSION=\"$(VERSION)\" 

configuration_files = \
	test_07.conf  \
	test_08.conf  \
	test_09.conf  \
	test_10.conf  \
	test_11.conf  \
	test_12.conf  \
	test_12a.conf \
	test_13.conf  \
	test_13a.conf \
	test_14.conf  \
	test_15.conf  \
	test_16.conf

SUBDIRS = test_11_module

EXTRA_DIST = $(configuration_files) suppressions

# replace with bin_PROGRAMS to check performance
bin_PROGRAMS = test_01 

test_01_SOURCES = test_01.c
test_01_LDADD   = ../src/libturbulence.la ../modules/mod-sasl/common-sasl.o $(AXL_LIBS) $(VORTEX_LIBS) $(VORTEX_SASL_LIBS) $(VORTEX_XML_RPC_LIBS) $(EXARG_LIBS) $(VORTEX_TLS_LIBS)

leak-check:
	libtool --mode=execute valgrind --leak-check=yes ./test_01


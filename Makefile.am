# additional content
EXTRA_DIST = VERSION turbulence.pc.in turbulence-config.in

# build the BEEP application server
if ENABLE_AF_ARCH_DOC
DOC_DIR = doc
endif

SUBDIRS = src data modules tools test $(DOC_DIR) 

installdir = $(prefix)/bin/

install_SCRIPTS = turbulence-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = turbulence.pc




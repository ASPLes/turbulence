# build the BEEP application server
if ENABLE_AF_ARCH_DOC
DOC_DIR = doc
endif

SUBDIRS = src data modules tools $(DOC_DIR) 

installdir = $(prefix)/sbin/

install_SCRIPTS = turbulence-config


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = turbulence.pc


EXTRA_DIST = 

# profile a flag to enable the source to code to react agaisnt the
# pcre inclusion.
if ENABLE_PCRE_SUPPORT
INCLUDE_PCRE_SUPPORT=-DENABLE_PCRE_SUPPORT
endif

if ENABLE_TERMIOS
INCLUDE_TERMIOS=-DENABLE_TERMIOS
endif

INCLUDES = -DCOMPILATION_DATE=`date +%s` -D__COMPILING_TURBULENCE__\
	   -DVERSION=\"$(TURBULENCE_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS)\
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS = turbulence

turbulence_SOURCES = \
	main.c 

turbulence_LDFLAGS = -Wl,-export-dynamic -ldl

turbulence_LDADD = $(AXL_LIBS) $(VORTEX_LIBS) $(EXARG_LIBS) libturbulence.la

lib_LTLIBRARIES          = libturbulence.la

libturbulenceinclude_HEADERS    = \
	turbulence-moddef.h \
	turbulence.h \
	turbulence-run.h \
	turbulence-module.h \
	turbulence-log.h \
	turbulence-ppath.h \
	turbulence-config.h \
	turbulence-db-list.h

libturbulence_la_SOURCES = \
	turbulence.c \
	turbulence-config.c \
	turbulence-run.c \
	turbulence-module.c \
	turbulence-log.c \
	turbulence-ppath.c \
	turbulence-db-list.c

libturbulence_la_LIBADD = $(AXL_LIBS) $(VORTEX_LIBS) $(EXARG_LIBS) $(PCRE_LIBS)
libturbulence_la_LDFLAGS = -Wl,-export-dynamic -ldl -no-undefined -export-symbols-regex '^(turbulence).*'

libturbulenceincludedir = $(includedir)/turbulence

libturbulence.def: update-def

update-def:
	echo "EXPORTS" > libturbulence.def
	cat .libs/libturbulence.exp | grep -v __turbulence >> libturbulence.def

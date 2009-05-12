EXTRA_DIST = libturbulence.def \
	exarg.h exarg.c \
	mod-turbulence.dtd.h \
	turbulence-config.dtd.h \
	turbulence-db-list.dtd.h

# profile a flag to enable the source to code to react agaisnt the
# pcre inclusion.
if ENABLE_PCRE_SUPPORT
INCLUDE_PCRE_SUPPORT=-DENABLE_PCRE_SUPPORT
endif

if ENABLE_TERMIOS
INCLUDE_TERMIOS=-DENABLE_TERMIOS
endif

INCLUDES = -DCOMPILATION_DATE=`date +%s` -D__COMPILING_TURBULENCE__ $(EXARG_FLAGS) \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" -DVORTEX_VERSION=\"$(VORTEX_VERSION)\" -DAXL_VERSION=\"$(AXL_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DPIDFILE=\""$(statusdir)/turbulence.pid"\" \
	   -DTBC_DATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS)\
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS) -g -Wall -ansi

bin_PROGRAMS = turbulence

turbulence_SOURCES = \
	main.c 

turbulence_LDFLAGS = -Wl,-export-dynamic -ldl

turbulence_LDADD = $(AXL_LIBS) $(VORTEX_LIBS) $(EXARG_LIBS) libturbulence.la

lib_LTLIBRARIES  = libturbulence.la

libturbulenceinclude_HEADERS    = \
	turbulence-expr.h \
	turbulence-moddef.h \
	turbulence.h \
	turbulence-ctx.h \
	turbulence-ctx-private.h \
	turbulence-handlers.h \
	turbulence-run.h \
	turbulence-module.h \
	turbulence-log.h \
	turbulence-ppath.h \
	turbulence-config.h \
	turbulence-db-list.h \
	turbulence-conn-mgr.h \
	turbulence-signal.h 

libturbulence_la_SOURCES = \
	exarg.c \
	turbulence.c \
	turbulence-ctx.c \
	turbulence-config.c \
	turbulence-run.c \
	turbulence-module.c \
	turbulence-log.c \
	turbulence-ppath.c \
	turbulence-db-list.c \
	turbulence-conn-mgr.c \
	turbulence-signal.c \
	turbulence-expr.c

libturbulence_la_LIBADD = $(AXL_LIBS) $(VORTEX_LIBS) $(PCRE_LIBS)
libturbulence_la_LDFLAGS = -Wl,-export-dynamic -ldl -no-undefined -export-symbols-regex '^(turbulence).*'

libturbulenceincludedir = $(includedir)/turbulence

libturbulence.def: update-def

update-def:
	echo "EXPORTS" > libturbulence.def
	cat .libs/libturbulence.exp | grep -v __turbulence >> libturbulence.def

update-dtds: mod-turbulence.dtd.h turbulence-config.dtd.h turbulence-db-list.dtd.h

mod-turbulence.dtd.h: ../data/mod-turbulence.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer
turbulence-config.dtd.h: ../data/turbulence-config.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer
turbulence-db-list.dtd.h: ../data/turbulence-db-list.dtd
	axl-knife --input $< --dtd-to-c --output $@ --ifnewer



# copy exarg dependency to not force people to download a library with
# two files
exarg.h exarg.c: 
	test -e exarg.h || cp ../../libexarg/src/exarg.h .
	test -e exarg.c || cp ../../libexarg/src/exarg.c .

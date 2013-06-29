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

INCLUDES = $(compiler_options) -DCOMPILATION_DATE=`date +%s` -D__COMPILING_TURBULENCE__ -D_POSIX_C_SOURCE  \
	   -DVERSION=\"$(TURBULENCE_VERSION)\" -DVORTEX_VERSION=\"$(VORTEX_VERSION)\" -DAXL_VERSION=\"$(AXL_VERSION)\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" -DDEFINE_CHROOT_PROTO -DDEFINE_KILL_PROTO -DDEFINE_MKSTEMP_PROTO \
	   -DPIDFILE=\""$(statusdir)/turbulence.pid"\" \
	   -DTBC_RUNTIME_DATADIR=\""$(runtimedatadir)"\" \
	   -DTBC_DATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS) $(EXARG_FLAGS) \
	   -D__TURBULENCE_ENABLE_DEBUG_CODE__ \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS)  -g -Wall -Werror -Wstrict-prototypes 

bin_PROGRAMS = turbulence


turbulence_SOURCES = \
	main-common.c main-common.h main.c

turbulence_LDFLAGS = -Wl,-export-dynamic -ldl

turbulence_LDADD = $(AXL_LIBS) $(VORTEX_LIBS) libturbulence.la

lib_LTLIBRARIES  = libturbulence.la

libturbulenceinclude_HEADERS    = \
	exarg.h \
	turbulence-types.h \
	turbulence-expr.h \
	turbulence-moddef.h \
	turbulence-support.h \
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
	turbulence-signal.h \
	turbulence-process.h \
	turbulence-loop.h \
	turbulence-mediator.h \
	turbulence-child.h 

libturbulence_la_SOURCES = \
	exarg.c \
	turbulence.c \
	turbulence-ctx.c \
	turbulence-config.c \
	turbulence-run.c \
	turbulence-support.c \
	turbulence-module.c \
	turbulence-log.c \
	turbulence-ppath.c \
	turbulence-db-list.c \
	turbulence-conn-mgr.c \
	turbulence-signal.c \
	turbulence-expr.c \
	turbulence-process.c \
	turbulence-loop.c \
	turbulence-mediator.c \
	turbulence-child.c 

libturbulence_la_LIBADD = $(AXL_LIBS) $(VORTEX_LIBS) $(PCRE_LIBS)
libturbulence_la_LDFLAGS = -Wl,-export-dynamic -ldl -no-undefined -export-symbols-regex '^(turbulence|__turbulence|exarg).*'

libturbulenceincludedir = $(includedir)/turbulence

all-local: update-def

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

update-exarg:
	cp ../../libexarg/src/exarg.[ch] .

# create directory and set permissions
install-exec-hook:
	@echo "Creating shared dir ${DESTDIR}${runtimedatadir}/turbulence.."
	install -d -m 700 ${DESTDIR}${runtimedatadir}/turbulence

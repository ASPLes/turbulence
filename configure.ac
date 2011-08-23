dnl  Turbulence:  BEEP application server
dnl  Copyright (C) 2007 Advanced Software Production Line, S.L.
dnl
dnl  This program is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License
dnl  as published by the Free Software Foundation; version 2.1 of
dnl  the License. 
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
dnl  GNU Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with this program; if not, write to the Free
dnl  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl  
dnl  You may find a copy of the license under this software is released
dnl  at COPYING file. This is LGPL software: you are wellcome to
dnl  develop propietary applications using this library withtout any
dnl  royalty or fee but returning back any change, improvement or
dnl  addition in the form of source code, project image, documentation
dnl  patches, etc. 
dnl
dnl  For comercial support on build BEEP enabled solutions contact us: 
dnl          
dnl      Postal address:
dnl         Advanced Software Production Line, S.L.
dnl         C/ Dr. Michavila Nº 14
dnl         Coslada 28820 Madrid
dnl         Spain
dnl
dnl      Email address:
dnl         info@aspl.es - http://fact.aspl.es
dnl

AC_INIT(turbulence,0.5.3.b4744.g4745)

TURBULENCE_VERSION=`cat VERSION`
AC_SUBST(TURBULENCE_VERSION)

AC_CONFIG_AUX_DIR(.)
dnl allow including long file names 
dnl AM_INIT_AUTOMAKE(turbulence, $TURBULENCE_VERSION)
AM_INIT_AUTOMAKE([no-define tar-ustar])

AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

AC_CHECK_HEADER(termios.h, [termios_found=yes], [termios_found=no])
AM_CONDITIONAL(ENABLE_TERMIOS, test ".$termios_found" = ".yes")

dnl external dependencies
PKG_CHECK_MODULES(AXL, axl >= 0.6.3) 

dnl general libries subsitution
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)
AXL_VERSION=`pkg-config --modversion axl`
AC_SUBST(AXL_VERSION)

dnl external dependencies
PKG_CHECK_MODULES(VORTEX, vortex-1.1 >= 1.1.5) 

dnl external dependency: check for base vortex 
AC_SUBST(VORTEX_CFLAGS)
AC_SUBST(VORTEX_LIBS)
VORTEX_VERSION=`pkg-config --modversion vortex-1.1`
AC_SUBST(VORTEX_VERSION)

dnl external dependencies: vortex-tunnel
PKG_CHECK_MODULES(VORTEX_TUNNEL, vortex-tunnel-1.1 >= 1.1.4) 
AC_SUBST(VORTEX_TUNNEL_CFLAGS)
AC_SUBST(VORTEX_TUNNEL_LIBS)
VORTEX_TUNNEL_VERSION=`pkg-config --modversion vortex-tunnel-1.1`
AC_SUBST(VORTEX_TUNNEL_VERSION)

dnl external dependencies: vortex-xml-rpc
PKG_CHECK_MODULES(VORTEX_XML_RPC, vortex-xml-rpc-1.1 >= 1.1.4) 
AC_SUBST(VORTEX_XML_RPC_CFLAGS)
AC_SUBST(VORTEX_XML_RPC_LIBS)
VORTEX_XML_RPC_VERSION=`pkg-config --modversion vortex-xml-rpc-1.1`
AC_SUBST(VORTEX_XML_RPC_VERSION)

dnl external dependencies: vortex-sasl
PKG_CHECK_MODULES(VORTEX_SASL, vortex-sasl-1.1 >= 1.1.4) 
AC_SUBST(VORTEX_SASL_CFLAGS)
AC_SUBST(VORTEX_SASL_LIBS)
VORTEX_SASL_VERSION=`pkg-config --modversion vortex-sasl-1.1`
AC_SUBST(VORTEX_SASL_VERSION)

dnl external dependencies: vortex-sasl
PKG_CHECK_MODULES(VORTEX_TLS, vortex-tls-1.1 >= 1.1.4) 
AC_SUBST(VORTEX_TLS_CFLAGS)
AC_SUBST(VORTEX_TLS_LIBS)
VORTEX_TLS_VERSION=`pkg-config --modversion vortex-tls-1.1`
AC_SUBST(VORTEX_TLS_VERSION)

dnl define a m4 macro to check for perl regular expresion support
AC_DEFUN([CHECK_LIB_PCRE],[dnl
AC_MSG_CHECKING([lib pcre])
AC_ARG_WITH(pcre,
[  --with-pcre[[=prefix]] builds tuburlence with regular expression support (highly recomended)],,
     with_pcre="yes")
if test ".$with_pcre" = ".no" ; then
  AC_MSG_RESULT([disabled])
  m4_ifval($2,$2)
else
  AC_MSG_RESULT([(testing)])
  AC_CHECK_LIB(pcre, pcre_study)
  if test "$ac_cv_lib_pcre_pcre_study" = "yes" ; then
     PCRE_LIBS="-lpcre"
     AC_MSG_CHECKING([lib pcre])
     AC_MSG_RESULT([$PCRE_LIBS])
     m4_ifval($1,$1)
  else
     OLDLDFLAGS="$LDFLAGS" ; LDFLAGS="$LDFLAGS -L$with_pcre/lib"
     OLDCPPFLAGS="$CPPFLAGS" ; CPPFLAGS="$CPPFLAGS -I$with_pcre/include"
     AC_CHECK_LIB(pcre, pcre_compile)
     CPPFLAGS="$OLDCPPFLAGS"
     LDFLAGS="$OLDLDFLAGS"
     if test "$ac_cv_lib_pcre_pcre_compile" = "yes" ; then
        AC_MSG_RESULT(.setting PCRE_LIBS -L$with_pcre/lib -lpcre)
        PCRE_LIBS="-L$with_pcre/lib -lpcre"
        test -d "$with_pcre/include" && PCRE_CFLAGS="-I$with_pcre/include"
        AC_MSG_CHECKING([lib pcre])
        AC_MSG_RESULT([$PCRE_LIBS])
        m4_ifval($1,$1)
     else
        AC_MSG_CHECKING([lib pcre])
	AC_MSG_ERROR([Cannot build turbulence without PCRE. This is mandatory (libpcre3-dev)])
        m4_ifval($2,$2)
     fi
  fi
fi
AC_SUBST([PCRE_LIBS])
AC_SUBST([PCRE_CFLAGS])
])

dnl check for libpcre support
CHECK_LIB_PCRE([pcre_found="yes"],[pcre_found="no"])
AM_CONDITIONAL(ENABLE_PCRE_SUPPORT, test ".$pcre_found" = ".yes")

dnl check for doxygen documentation
AC_ARG_ENABLE(turbulence-doc, [  --enable-turbulence-doc    Enable building Turbulence documentation (doxygen required) [default=yes]], 
	      enable_turbulence_doc="$enableval", 
	      enable_turbulence_doc=yes)
if test x$enable_turbulence_doc = xyes ; then
        AC_CHECK_PROG(DOXYGEN, doxygen, "yes", "no")
fi
AM_CONDITIONAL(ENABLE_AF_ARCH_DOC, test "x$DOXYGEN" = "xyes")

dnl check to build tbc-mod-gen tool
AC_ARG_ENABLE(enable-tbc-mod-gen, [  --enable-tbc-mod-gen       Enable building tbc-mod-gen [default=yes]], 
	      enable_tbc_mod_gen="$enableval", 
	      enable_tbc_mod_gen=yes)
AM_CONDITIONAL(ENABLE_TBC_MOD_GEN, test "x$enable_tbc_mod_gen" = "xyes")

dnl check to build tbc-dblist-mgr tool
AC_ARG_ENABLE(enable-tbc-dblist-mgr, [  --enable-tbc-dblist-mgr    Enable building tbc-dblist-mgr [default=yes]], 
	      enable_tbc_dblist_mgr="$enableval", 
	      enable_tbc_dblist_mgr=yes)
AM_CONDITIONAL(ENABLE_TBC_DBLIST_MGR, test "x$enable_tbc_dblist_mgr" = "xyes")

dnl check to build tbc-ctl tool
AC_ARG_ENABLE(enable-tbc-ctl, [  --enable-tbc-ctl    Enable building tbc-ctl (CLI tool) [default=yes]], 
	      enable_tbc_ctl="$enableval", 
	      enable_tbc_ctl=yes)
if test x$enable_tbc_ctl = xyes ; then
   AC_CHECK_HEADER(readline/readline.h, readline_found=true, readline_found=false)
   AC_TRY_LINK(,,[	READLINE_DEPLIBS="-lreadline" ],[ 
		LIBS="$LIBS -ltermcap" 
		AC_TRY_LINK(,,[ READLINE_DEPLIBS="-lreadline -ltermcap" ],[
			AC_MSG_ERROR([Cannot figure out how to link with the readline library; see config.log for more information])
		])
	])
   AM_CONDITIONAL(ENABLE_READLINE, test "x$readline_found" = "xtrue")

   AC_ARG_VAR(READLINE_PATH, [Readline base include path. This can be helpful to set a different location for a readline installation.
This will be used to build a include path as READLINE_PATH/include and a READLINE_PATH/lib.
])
   AM_CONDITIONAL(ENABLE_TBC_CTL, test "x$enable_tbc_ctl" = "xyes")
fi

dnl check for statusdir
AC_ARG_ENABLE(status-dir, [  --status-dir=DIR          Allows to configure the status directory. 
                                This directory is used to place pid file and status info. 
                                Default: /var/run],
              statusdir="$enableval",
	      statusdir="/var/run")
AC_SUBST(statusdir)

dnl check for statusdir
AC_ARG_ENABLE(runtime-data-dir, [  --runtime-data-dir=DIR          Allows to configure the runtime created data directory. 
                                This directory is used to place some files like unix sockets and temporal files created
                                during Turbulence function. This is a prefix directory.
                                Default: /var/lib],
              runtimedatadir="$enableval",
	      runtimedatadir="/var/lib")
AC_SUBST(runtimedatadir)

# support to check some dependencies for exarg library embeded into
AC_TRY_LINK([#define _GNU_SOURCE
#include <stdio.h>], 
[
  char * result;
  return vasprintf (&result, "This is a test: %d", NULL);
], [have_vasprintf=yes],[have_vasprintf=no])
echo "Checking vasprintf support: $have_vasprintf"

dnl support for vasprintf
case $have_vasprintf in
yes)
	EXARG_FLAGS="-DHAVE_VASPRINTF $EXARG_FLAGS"
	echo "vasprintf support found"
     ;;
esac

dnl check for windows platform
case $host in
*-*-mingw*)
	EXARG_FLAGS="-DOS_WIN32 $EXARG_FLAGS"
	echo "windows platform found"
	;;	
*)
	echo "platform found: $host"
    ;;
esac
AC_SUBST(EXARG_FLAGS)

# get python includes 
PYTHON_INCLUDES=`python-config --includes 2>/dev/null`
old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-Wall -Werror $PYTHON_INCLUDES"
echo "Python includes: $PYTHON_INCLUDES"
AC_SUBST(PYTHON_INCLUDES)

AC_TRY_COMPILE([#include <Python.h>],
  [Py_InitModule3(NULL, NULL, NULL);],
  python_devel_found=yes,
  python_devel_found=no)
AC_MSG_RESULT([Python development headers status: $python_devel_found])
if test "x$python_devel_found" = "xno"; then
   AC_MSG_WARN([Cannot find Python.h header or current python development environment do not compile symbols required. On debian try to install python-dev package.])
fi
AM_CONDITIONAL(ENABLE_MOD_PYTHON, test "x$python_devel_found" = "xyes")

dnl get python flags and libs
PYTHON_CFLAGS=`python-config --cflags 2>/dev/null`
PYTHON_LIBS=`python-config --ldflags 2>/dev/null`
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBS)

dnl get py-vortex flags
PKG_CHECK_MODULES(PY_VORTEX, py-vortex >= 0.3.8 py-vortex-sasl >= 0.3.8 py-vortex-tls >= 0.3.8) 
AC_SUBST(PY_VORTEX_CFLAGS)
AC_SUBST(PY_VORTEX_LIBS)

dnl mod-sasl-mysql flags
MYSQL_CFLAGS=`mysql_config --cflags 2>/dev/null`
MYSQL_LIBS=`mysql_config --libs 2>/dev/null`
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)
AC_TRY_COMPILE([#include <mysql/mysql.h>
#include <stdlib.h>],
  [mysql_init(NULL);],
  mysql_devel_found=yes,
  mysql_devel_found=no)
AC_MSG_RESULT([MySQL development headers status: $mysql_devel_found])
if test "x$mysql_devel_found" = "xno"; then
   AC_MSG_WARN([Cannot find mysql.h header or current MySQL development environment do not compile symbols required. On debian try to install libmysqlclient15-dev package.])
fi
AM_CONDITIONAL(ENABLE_MYSQL_SUPPORT, test "x$mysql_devel_found" = "xyes")

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
doc/Makefile
doc/turbulence-doc.doxygen
modules/Makefile
modules/mod-test/Makefile
modules/mod-test/mod-test.xml
modules/mod-tunnel/Makefile
modules/mod-tunnel/mod-tunnel.xml
modules/mod-sasl/Makefile
modules/mod-sasl/mod-sasl.xml
modules/mod-sasl/sasl-radmin.pc
modules/mod-python/Makefile
modules/mod-python/doc/Makefile
modules/mod-python/doc/conf.py
modules/mod-radmin/Makefile
modules/mod-tls/Makefile
tools/Makefile
tools/tbc-mod-gen/Makefile
tools/tbc-dblist-mgr/Makefile
tools/tbc-ctl/Makefile
test/Makefile
test/test_10b_module/Makefile
test/test_11_module/Makefile
test/test_15_module/Makefile
turbulence.pc
turbulence-config
])

dnl Ensure turbulence-config is executable
chmod +x ./turbulence-config

echo "------------------------------------------"
echo "--       Turbulence Settings            --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "   Status directory:               [$statusdir]"
echo "   Runtime data directory:         [$runtimedatadir]"
echo "   Reg. expression support (pcre): [$pcre_found]"
if test "$pcre_found" = "no" ; then
	echo "NOTE: building turbulence without pcre support will"
	echo "      disable powerful features on profile path."
	echo "      Pcre is really recomended!!!"
fi
echo "   Build tbc-sasl-conf:            [$termios_found]"
echo "   Build tbc-mod-gen:              [$enable_tbc_mod_gen]"
echo "   Build tbc-dblist-mgr:           [$enable_tbc_dblist_mgr]"
echo "   Build tbc-ctl:                  [$enable_tbc_ctl]"
echo "   Build mod-python:               [$python_devel_found]"
if test "x$python_devel_found" = "xyes"; then
echo "         (flags: $PYTHON_CFLAGS, libs: $PYTHON_LIBS)"
fi
echo "   Build with MySQL support:       [$mysql_devel_found]"
if test "x$mysql_devel_found" = "xyes"; then
echo "         (flags: $MYSQL_CFLAGS, libs: $MYSQL_LIBS)"
fi
echo "   Build turbulence documentation  [$DOXYGEN]"
echo "------------------------------------------"
echo "--            Let it BEEP!              --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

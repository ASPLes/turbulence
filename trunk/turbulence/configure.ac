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

AC_INIT(src/main.c)

TURBULENCE_VERSION=`cat VERSION`
AC_SUBST(TURBULENCE_VERSION)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(turbulence, $TURBULENCE_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

AC_CHECK_HEADER(termios.h, [termios_found=yes], [termios_found=no])
AM_CONDITIONAL(ENABLE_TERMIOS, test ".$termios_found" = ".yes")

dnl external dependencies
PKG_CHECK_MODULES(AXL, axl >= 0.4.8) 

dnl general libries subsitution
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)
AXL_VERSION=`pkg-config --modversion axl`
AC_SUBST(AXL_VERSION)

dnl external dependencies
PKG_CHECK_MODULES(VORTEX, vortex >= 1.0.7) 

dnl general libries subsitution
AC_SUBST(VORTEX_CFLAGS)
AC_SUBST(VORTEX_LIBS)
VORTEX_VERSION=`pkg-config --modversion vortex`
AC_SUBST(VORTEX_VERSION)

dnl external dependencies
PKG_CHECK_MODULES(EXARG, exarg) 

dnl general libries subsitution
AC_SUBST(EXARG_CFLAGS)
AC_SUBST(EXARG_LIBS)
EXARG_VERSION=`pkg-config --modversion exarg`
AC_SUBST(EXARG_VERSION)

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
        AC_MSG_RESULT([no, (WARNING: building turbulence without pcre!!)])
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
AC_ARG_ENABLE(turbulence-doc, [  --enable-turbulence-doc       Enable building Turbulence documentation (doxygen required) [default=yes]], 
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
AC_ARG_ENABLE(enable-tbc-dblist-mgr, [  --enable-tbc-dblist-mgr       Enable building tbc-dblist-mgr [default=yes]], 
	      enable_tbc_dblist_mgr="$enableval", 
	      enable_tbc_dblist_mgr=yes)
AM_CONDITIONAL(ENABLE_TBC_DBLIST_MGR, test "x$enable_tbc_dblist_mgr" = "xyes")


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
tools/Makefile
tools/tbc-mod-gen/Makefile
tools/tbc-dblist-mgr/Makefile
turbulence.pc
turbulence-config
])

echo "------------------------------------------"
echo "--       Turbulence Settings            --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "   Reg. expression support (pcre): [$pcre_found]"
echo "   Build tbc-sasl-conf:            [$termios_found]"
echo "   Build tbc-mod-gen:              [$enable_tbc_mod_gen]"
echo "   Build tbc-dblist-mgr:           [$enable_tbc_dblist_mgr]"
echo "   Build turbulence documentation  [$DOXYGEN]"
echo "------------------------------------------"
echo "--            Let it BEEP!              --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

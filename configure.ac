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

dnl external dependencies
PKG_CHECK_MODULES(AXL, axl >= 0.4.1) 

dnl general libries subsitution
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)

dnl external dependencies
PKG_CHECK_MODULES(VORTEX, vortex >= 1.0.1) 

dnl general libries subsitution
AC_SUBST(VORTEX_CFLAGS)
AC_SUBST(VORTEX_LIBS)

dnl external dependencies
PKG_CHECK_MODULES(EXARG, exarg) 

dnl general libries subsitution
AC_SUBST(EXARG_CFLAGS)
AC_SUBST(EXARG_LIBS)



AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
modules/Makefile
modules/mod-test/Makefile
modules/mod-test/mod-test.xml
])

echo "------------------------------------------"
echo "--       Turbulence Settings            --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo
echo "------------------------------------------"
echo "--            Let it beep!              --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

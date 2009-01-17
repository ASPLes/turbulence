#!/bin/sh
#
# Turbulence:  BEEP application server
# Copyright (C) 2007 Advanced Software Production Line, S.L.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; version 2.1 of
# the License. 
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
# 
# You may find a copy of the license under this software is released
# at COPYING file. This is LGPL software: you are wellcome to
# develop propietary applications using this library withtout any
# royalty or fee but returning back any change, improvement or
# addition in the form of source code, project image, documentation
# patches, etc. 
#
# For comercial support on build BEEP enabled solutions contact us:
#         
#     Postal address:
#        Advanced Software Production Line, S.L.
#        C/ Dr. Michavila N� 14
#        Coslada 28820 Madrid
#        Spain
#
#     Email address:
#        info@aspl.es - http://fact.aspl.es
#

PACKAGE="Turbulence:  BEEP application server"

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile $PACKAGE";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile $PACKAGE";
	echo;
	exit;
}

echo "Generating configuration files for $PACKAGE, please wait...." 
echo; 

touch NEWS README AUTHORS ChangeLog 
libtoolize --force;
aclocal $ACLOCAL_FLAGS; 
autoheader --warnings=error;
automake --add-missing --Werror;
autoconf --force --warnings=error;


./configure $@ --enable-maintainer-mode --enable-compile-warnings
/*  tbc-mod-gen: A tool to produce modules for the Turbulence BEEP server
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *
 *  For comercial support on build BEEP enabled solutions, supporting
 *  turbulence based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.turbulence.ws
 */
#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#include <turbulence.h>

void    support_open_file (const char * format, ...);

void    support_close_file      ();

#define write support_write

#define write_sl support_sl_write

void    support_write (const char * format, ...);

void    support_sl_write        (const char * format, ...);

#define push_indent support_push_indent

void    support_push_indent     ();

#define pop_indent support_pop_indent

void    support_pop_indent      ();

bool    support_are_equal           (char * file1 , 
				     char * file2);

void    support_move_file           (char * from, 
				     char * to);

bool    support_dump_file           (axlDoc * doc, 
				     int tabular, 
				     const char * format_path, ...);

char  * support_clean_name          (const char * name);

char  * support_to_upper            (const char * name);

char  * support_to_lower            (const char * name);

void    support_make_executable     (const char * format, ...);

#endif

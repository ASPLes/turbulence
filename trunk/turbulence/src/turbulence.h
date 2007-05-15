/*  Turbulence:  BEEP application server
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are wellcome to
 *  develop propietary applications using this library withtout any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc.
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
 *         info@aspl.es - http://fact.aspl.es
 */
#ifndef __TURBULENCE_H__
#define __TURBULENCE_H__

/* system includes */
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>

/* BEEP support */
#include <vortex.h>

/* XML support */
#include <axl.h>

/* command line argument parsing support */
#include <exarg.h>

/* local includes */
#include <turbulence-moddef.h>
#include <turbulence-config.h>
#include <turbulence-run.h>
#include <turbulence-module.h>
#include <turbulence-log.h>
#include <turbulence-ppath.h>

/* definitions */

void turbulence_set_console_debug (bool debug);

/** 
 * Drops to the console stderr an error msg, placing the content
 * prefixed with the file and the line that caused the error.
 *
 * To drop an error message use:
 * \code
 *   error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define error(m,...) do{turbulence_error (__AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_error (const char * file, int line, const char * format, ...);

/** 
 * Drops to the console stdout a msg, placing the content prefixed
 * with the file and the line that caused the message.
 *
 * To drop a message use:
 * \code
 *   msg ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg(m,...)   do{turbulence_msg (__AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_msg   (const char * file, int line, const char * format, ...);

/** 
 * Drops to the console stdout a warning, placing the content prefixed
 * with the file and the line that caused the message.
 *
 * To drop a message use:
 * \code
 *   wrn ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn(m,...)   do{turbulence_msg (__AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_wrn   (const char * file, int line, const char * format, ...);


bool turbulence_init (int argc, char ** argv);

void turbulence_exit (int value); 

void turbulence_cleanup ();

void turbulence_reload_config (int value);

/** 
 * @brief Available tests to be performed while using \ref
 * camuc_file_test
 */
typedef enum {
	/** 
	 * @brief Check if the path exist.
	 */
	FILE_EXISTS     = 1 << 0,
	/** 
	 * @brief Check if the path provided is a symlink.
	 */
	FILE_IS_LINK    = 1 << 1,
	/** 
	 * @brief Check if the path provided is a directory.
	 */
	FILE_IS_DIR     = 1 << 2,
	/** 
	 * @brief Check if the path provided is a regular file.
	 */
	FILE_IS_REGULAR = 1 << 3,
} FileTest;


bool     turbulence_file_test   (const char * path,   
				 FileTest test);

bool     turbulence_file_test_v (const char * format, 
				 FileTest test, ...);

long int turbulence_last_modification (const char * file);

typedef enum {
	DISABLE_STDIN_ECHO = 1 << 0,
} TurbulenceIoFlags;

char *   turbulence_io_get (char * prompt, TurbulenceIoFlags flags);


#endif

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
#include <turbulence.h>

#define HELP_HEADER "Turbulence: BEEP application server\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/** 
 * Starts turbulence execution, initializing all libraries required by
 * the server application.
 *
 * A call to turbulence_exit is required before exit.
 */
bool turbulence_init (int argc, char ** argv)
{
	/*** init exarg library ***/

	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("config", "f", EXARG_STRING, 
			   "Main server configuration location.");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/*** init the vortex library ***/
	if (! vortex_init ()) {
		error ("unable to start vortex library, terminating turbulence execution..");
		return false;
	} /* end if */

	/*** not required to initialize axl library, already done by vortex ***/
	msg ("turbulence internal init");

	/* init ok */
	return true;
} /* end if */

/** 
 * Terminates the turbulence excution, returing the exit value
 * provided as first parameter.
 * 
 * @param value The exit code to return.
 */
void turbulence_exit (int value)
{
	

	/* terminate */
	exit (value);

	return;
}

/** 
 * @internal function that actually handles the console error.
 */
void __error (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	fprintf (stderr, "[ error ] (%s:%d) ", file, line);
	
	va_start (args, format);
	vfprintf (stderr, format, args);
	va_end (args);

	fprintf (stderr, "\n");
	
	fflush (stderr);
	
	return;
}

/** 
 * @internal function that actually handles the console msg.
 */
void __msg (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	fprintf (stdout, "[  msg  ] (%s:%d) ", file, line);
	
	va_start (args, format);
	vfprintf (stdout, format, args);
	va_end (args);

	fprintf (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

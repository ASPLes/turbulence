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
 *         info@aspl.es - http://www.turbulence.ws
 */
#include <turbulence.h>

static FILE * __general_log    = NULL;
static FILE * __error_log      = NULL;
static FILE * __vortex_log     = NULL;
static FILE * __access_log     = NULL;

/** 
 * @brief Init the turbulence log module.
 */
void turbulence_log_init ()
{
	/* get current turbulence configuration */
	axlDoc  * doc = turbulence_config_get ();
	axlNode * node;

	/* check log reporting */
	node = axl_doc_get (doc, "/turbulence/global-settings/log-reporting");
	if (! HAS_ATTR_VALUE (node, "enabled", "yes")) {
		msg ("log reporting disabled");
		return;
	}

	/* open all logs */
	node      = axl_node_get_child_called (node, "general-log");
	__general_log = fopen (ATTR_VALUE (node, "file"), "a");
	if (__general_log == NULL) {
		error ("unable to open general log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open error logs */
	node      = axl_node_get_child_called (node, "error-log");
	__error_log = fopen (ATTR_VALUE (node, "file"), "a");
	if (__error_log == NULL) {
		error ("unable to open error log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open access log */
	node      = axl_node_get_child_called (node, "access-log");
	__access_log  = fopen (ATTR_VALUE (node, "file"), "a");
	if (__access_log == NULL) {
		error ("unable to open access log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	node      = axl_node_get_child_called (node, "vortex-log");
	__vortex_log  = fopen (ATTR_VALUE (node, "file"), "a");
	if (__vortex_log == NULL) {
		error ("unable to open vortex log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);
	
	return;
}

/** 
 * @internal macro that allows to report a message to the particular
 * log, appending date information.
 */
#define REPORT(log, message, args, file, line) do{                 \
  if (log) {                                                       \
     time_val = time (NULL);                                       \
     time_str = axl_strdup (ctime (&time_val));                    \
     time_str [strlen (time_str) - 1] = 0;                         \
     fprintf (log, "%s (%s:%d) ", time_str, file, line);           \
     axl_free (time_str);                                          \
     vfprintf (log, message, args);                                \
     fprintf  (log, "\n");                                         \
     fflush (log);                                                 \
  }                                                                \
} while (0);

/** 
 * @brief Reports a single line to the particular log, configured by
 * "type".
 * 
 * @param type The log to select for reporting.
 *
 * @param message The message to report.
 */
void turbulence_log_report (LogReportType type, 
			    const char * message, 
			    va_list args,
			    const char * file,
			    int          line)
{
	time_t   time_val;
	char   * time_str;

	/* according to the type received report */
	if ((type & LOG_REPORT_GENERAL) == LOG_REPORT_GENERAL) 
		REPORT (__general_log, message, args, file, line);
	
	if ((type & LOG_REPORT_ERROR) == LOG_REPORT_ERROR) 
		REPORT (__error_log, message, args, file, line);
	
	if ((type & LOG_REPORT_ACCESS) == LOG_REPORT_ACCESS) 
		REPORT (__access_log, message, args, file, line);

	if ((type & LOG_REPORT_VORTEX) == LOG_REPORT_VORTEX) 
		REPORT (__vortex_log, message, args, file, line);
	return;
}

/** 
 * @brief Stops and dealloc all resources hold by the module.
 */
void turbulence_log_cleanup ()
{
	if (__general_log)
		fclose (__general_log);
	if (__error_log)
		fclose (__error_log);
	if (__access_log)
		fclose (__access_log);
	if (__vortex_log)
		fclose (__vortex_log);

	return;
}


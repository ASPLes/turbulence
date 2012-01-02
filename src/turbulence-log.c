/*  Turbulence BEEP application server
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions, supporting
 *  turbulence based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/turbulence
 */
#include <turbulence.h>
#include <stdlib.h>

/* local include */
#include <turbulence-ctx-private.h>

/** 
 * @brief Init the turbulence log module.
 */
void turbulence_log_init (TurbulenceCtx * ctx)
{
	/* get current turbulence configuration */
	axlDoc  * doc = turbulence_config_get (ctx);
	axlNode * node;

	/* check log reporting */
	node = axl_doc_get (doc, "/turbulence/global-settings/log-reporting");
	if (node == NULL) {
		abort_error ("Unable to find log configuration <turbulence/global-settings/log-reporting>");
		CLEAN_START(ctx);
		return;
	} /* end if */

	/* check enabled attribute */
	if (! HAS_ATTR (node, "enabled")) {
		abort_error ("Missing attribute 'enabled' located at <turbulence/global-settings/log-reporting>. Unable to determine if log is enabled");
		CLEAN_START(ctx);
		return;
	}

	/* check if log reporting is enabled or not */
	if (! HAS_ATTR_VALUE (node, "enabled", "yes")) {
		msg ("log reporting to file disabled");
		return;
	}

	/* open all logs */
	node      = axl_node_get_child_called (node, "general-log");
	/* check permission access */

	ctx->general_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->general_log == -1) {
		abort_error ("unable to open general log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open error logs */
	node      = axl_node_get_child_called (node, "error-log");
	ctx->error_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->error_log == -1) {
		abort_error ("unable to open error log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open access log */
	node      = axl_node_get_child_called (node, "access-log");
	ctx->access_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->access_log == -1) {
		abort_error ("unable to open access log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	node      = axl_node_get_child_called (node, "vortex-log");
	ctx->vortex_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->vortex_log == -1) {
		abort_error ("unable to open vortex log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);
	
	return;
}

/** 
 * @brief Allows to configure the provided file descriptor to receive
 * logs produced in the context of type parameter. The configuration
 * is perfomed on the context provided.
 *
 * @param ctx Turbulence context to be configured.
 * @param type The log context to be reconfigured.
 * @param descriptor The descriptor to be configured.
 */
void      turbulence_log_configure (TurbulenceCtx * ctx,
				    LogReportType   type,
				    int             descriptor)
{
	v_return_if_fail (ctx);
	switch (type) {
	case LOG_REPORT_GENERAL:
		/* configure new general log descriptor */
		ctx->general_log = descriptor;
		break;
	case LOG_REPORT_ERROR:
		/* configure new error log descriptor */
		ctx->error_log = descriptor;
		break;
	case LOG_REPORT_ACCESS:
		/* configure new access log descriptor */
		ctx->access_log = descriptor;
		break;
	case LOG_REPORT_VORTEX:
		/* configure new vortex log descriptor */
		ctx->vortex_log = descriptor;
		break;
	} /* end switch */
	return;
}


axl_bool __turbulence_log_manager_transfer_content (TurbulenceLoop * loop, 
						    TurbulenceCtx  * ctx,
						    int              descriptor,
						    axlPointer       ptr,
						    axlPointer       ptr2)
{
	int     size;
	int     size_written;
	char    buffer[4097];
	int     output_sink = PTR_TO_INT (ptr);

	switch (output_sink) {
	case LOG_REPORT_GENERAL:
		output_sink = ctx->general_log;
		break;
	case LOG_REPORT_ERROR:
		output_sink = ctx->error_log;
		break;
	case LOG_REPORT_ACCESS:
		output_sink = ctx->access_log;
		break;
	case LOG_REPORT_VORTEX:
		output_sink = ctx->vortex_log;
		break;
	default:
		/* send to default output sink */
		output_sink = ctx->general_log;
	} /* end switch */

	/* read content */
	size = read (descriptor, buffer, 4096);
			
	/* check closed socket (child process finished) */
	if (size <= 0) 
		return axl_false;
			
	/* transfer content to the associated socket */
	buffer[size] = 0;
	size_written = write (output_sink, buffer, size);
	if (size_written != size) {
		error ("failed to write log received from child, content differs (%d != %d), error was: %s", 
		       size, size_written,
		       vortex_errno_get_last_error ());
	} /* end if */

	return axl_true;
}

/** 
 * @internal This function allows to start the log manager that will
 * control file descriptors to logs opened and will control pipes to
 * child process to write content received.
 */
void turbulence_log_manager_start (TurbulenceCtx * ctx)
{
	/* check if log is enabled */
	if (! turbulence_log_is_enabled (ctx)) {
		msg ("turbulence log disabled");
		return;
	}

	/* crear manager */
	ctx->log_manager = turbulence_loop_create (ctx);

	msg ("log manager started");
	return;
}

/** 
 * @internal Function used to register new descriptors that are
 * connected to local logs making all content received from this
 * descriptor to be redirected to the local file designated by type.
 */
void      turbulence_log_manager_register (TurbulenceCtx * ctx,
					   LogReportType   type,
					   int             descriptor)
{
	v_return_if_fail (ctx);

	/* create log descriptor linking the descriptor received to
	 * the current descriptor managing the context signaled by
	 * type */
	msg ("register fd: %d (type: %d)", descriptor, type);
	switch (type) {
	case LOG_REPORT_GENERAL:
		/* configure general log watcher */
		turbulence_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __turbulence_log_manager_transfer_content,
						  INT_TO_PTR (LOG_REPORT_GENERAL),
						  NULL);
		break;
	case LOG_REPORT_ERROR:
		/* configure error log watcher */
		turbulence_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __turbulence_log_manager_transfer_content,
						  INT_TO_PTR (LOG_REPORT_ERROR),
						  NULL);
		break;
	case LOG_REPORT_ACCESS:
		/* configure access log watcher */
		turbulence_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __turbulence_log_manager_transfer_content,
						  INT_TO_PTR(LOG_REPORT_ACCESS),
						  NULL);
		break;
	case LOG_REPORT_VORTEX:
		/* configure vortex log watcher */
		turbulence_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __turbulence_log_manager_transfer_content,
						  INT_TO_PTR(LOG_REPORT_VORTEX),
						  NULL);
		break;
	} /* end switch */
	return;
}


/** 
 * @internal macro that allows to report a message to the particular
 * log, appending date information.
 */
void REPORT (int log, const char * message, va_list args, const char * file, int line) 
{
	/* get turbulence context */
	time_t             time_val;
	char             * time_str;
	char             * string;
	char             * string2;
	char             * result;
	int                length;
	int                length2;
	int                total;

	/* do not report if log description is not defined */
	if (log < 0)
		return;

	/* create timestamp */
	time_val = time (NULL);
	time_str = axl_strdup (ctime (&time_val));
	if (time_str == NULL)
		return;
	time_str [strlen (time_str) - 1] = 0;

	/* write stamp */
	string = axl_strdup_printf ("%s [%d] (%s:%d) ", time_str, getpid (), file, line);
	axl_free (time_str);
	if (string == NULL)
		return;
	length = strlen (string);

	/* create message */
	string2 = axl_strdup_printfv (message, args);
	if (string2 == NULL) {
		axl_free (string);
		return;
	}
	length2 = strlen (string2);

	/* build final log message */
	total  = length + length2 + 1;
	result = axl_new (char, total + 1);
	if (result == NULL) {
		axl_free (string);
		axl_free (string2);
		return;
	}

	memcpy (result, string, length);
	memcpy (result + length, string2, length2);
	memcpy (result + length + length2, "\n", 1);

	axl_free (string);
	axl_free (string2);
	
	/* write content: do it in a single operation to avoid mixing
	 * content from different logs at the log file. */
	if (write (log, result, total) == -1) {
		axl_free (result);
		return;
	}
	
	/* release memory used */
	axl_free (result);
	return;
} 

/** 
 * @brief Reports a single line to the particular log, configured by
 * "type".
 * 
 * @param type The log to select for reporting. The function do not
 * support reporting at the same call to several targets. You must
 * call one time for each target to report.
 *
 * @param message The message to report.
 */
void turbulence_log_report (TurbulenceCtx   * ctx,
			    LogReportType     type, 
			    const char      * message, 
			    va_list           args,
			    const char      * file,
			    int               line)
{
	/* according to the type received report */
	if ((type & LOG_REPORT_GENERAL) == LOG_REPORT_GENERAL) 
		REPORT (ctx->general_log, message, args, file, line);
	
	if ((type & LOG_REPORT_ERROR) == LOG_REPORT_ERROR) 
		REPORT (ctx->error_log, message, args, file, line);
	
	if ((type & LOG_REPORT_ACCESS) == LOG_REPORT_ACCESS) 
		REPORT (ctx->access_log, message, args, file, line);

	if ((type & LOG_REPORT_VORTEX) == LOG_REPORT_VORTEX) 
		REPORT (ctx->vortex_log, message, args, file, line);
	return;
}

/** 
 * @brief Allows to check if the log to file is enabled on the
 * provided context.
 *
 * @param ctx The context where file log is checked to be enabled or
 * not.
 */
axl_bool   turbulence_log_is_enabled    (TurbulenceCtx * ctx)
{
	axlDoc  * config;
	axlNode * node;

	/* check context received */
	if (ctx == NULL)
		return axl_false;
	
	/* get configuration */
	config = turbulence_config_get (ctx);
	node   = axl_doc_get (config, "/turbulence/global-settings/log-reporting");

	/* check value returned */
	return turbulence_config_is_attr_positive (ctx, node, "enabled");
}

void __turbulence_log_close (TurbulenceCtx * ctx)
{
	/* close the general log */
	if (ctx->general_log >= 0)
		close (ctx->general_log);
	ctx->general_log = -1;

	/* close the error log */
	if (ctx->error_log >= 0)
		close (ctx->error_log);
	ctx->error_log = -1;

	/* close the access log */
	if (ctx->access_log >= 0)
		close (ctx->access_log);
	ctx->access_log = -1;

	/* close vortex log */
	if (ctx->vortex_log >= 0)
		close (ctx->vortex_log);
	ctx->vortex_log = -1;
	return;
}

void __turbulence_log_reopen (TurbulenceCtx * ctx)
{
	msg ("Reload received, reopening log references..");

	/* call to close all logs opened at this moment */
	__turbulence_log_close (ctx);

	/* call to open again */
	turbulence_log_init (ctx);

	msg ("Log reopening finished..");

	return;
}

/** 
 * @internal
 * @brief Stops and dealloc all resources hold by the module.
 */
void turbulence_log_cleanup (TurbulenceCtx * ctx)
{
	/* call to close current logs */
	__turbulence_log_close (ctx);

	/* now finish log manager */
	turbulence_loop_close (ctx->log_manager, axl_true);
	ctx->log_manager = NULL;

	return;
}



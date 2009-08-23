/*  Turbulence:  BEEP application server
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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

	ctx->general_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY);
	if (ctx->general_log == -1) {
		abort_error ("unable to open general log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open error logs */
	node      = axl_node_get_child_called (node, "error-log");
	ctx->error_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY);
	if (ctx->error_log == -1) {
		abort_error ("unable to open error log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open access log */
	node      = axl_node_get_child_called (node, "access-log");
	ctx->access_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY);
	if (ctx->access_log == -1) {
		abort_error ("unable to open access log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	node      = axl_node_get_child_called (node, "vortex-log");
	ctx->vortex_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY);
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

/** 
 * @internal Type definition used to associate the pipe or socket
 * (descriptor) that is connecting to a child process producing logs
 * that must be routed to a particular file descriptor on the current
 * process (output_sink).
 */
typedef struct _TurbulenceLogDescriptor {
	/* descriptor where content will be receved from child
	 * process */
	int descriptor;
	/* descriptor connecting to local log opened (maybe a file or
	 * something else) */
	int output_sink;
} TurbulenceLogDescriptor;

/* build file set to watch */
int __turbulence_log_build_watch_set (TurbulenceCtx * ctx)
{
	int                       max_fds = 0;
	TurbulenceLogDescriptor * log_descriptor;

	/* reset descriptor set */
	__vortex_io_waiting_default_clear (ctx->log_manager_fileset);
	
	/* reset cursor */
	axl_list_cursor_first (ctx->log_manager_cursor);
	while (axl_list_cursor_has_item (ctx->log_manager_cursor)) {
		/* get log descriptor */
		log_descriptor = axl_list_cursor_get (ctx->log_manager_cursor);

		/* now add to the waiting socket */
		if (! __vortex_io_waiting_default_add_to (log_descriptor->descriptor, 
							  NULL, 
							  ctx->log_manager_fileset)) {
			
			/* failed to add descriptor, close it and remove from wait list */
			axl_list_cursor_remove (ctx->log_manager_cursor);
			continue;
		} /* end if */

		/* compute max_fds */
		max_fds    = (log_descriptor->descriptor > max_fds) ? log_descriptor->descriptor: max_fds;
		
		/* get the next item */
		axl_list_cursor_next (ctx->log_manager_cursor);
	} /* end if */

	return max_fds;
}

void __turbulence_log_manager_descriptor_free (axlPointer __log_descriptor)
{
	TurbulenceLogDescriptor * log_descriptor = __log_descriptor;
	vortex_close_socket (log_descriptor->descriptor);
	axl_free (log_descriptor);
	return;
}

axl_bool __turbulence_log_manager_read_first (TurbulenceCtx * ctx)
{
	TurbulenceLogDescriptor * log_descriptor;

	log_descriptor = vortex_async_queue_pop (ctx->log_manager_queue);

	/* check item received: if null received terminate loop */
	if (PTR_TO_INT (log_descriptor) == -4)
		return axl_false;

	/* register log_descriptor on the list */
	axl_list_append (ctx->log_manager_list, log_descriptor);       

	return axl_true;
}

axl_bool __turbulence_log_manager_read_pending (TurbulenceCtx * ctx)
{
	TurbulenceLogDescriptor * log_descriptor;

	while (axl_true) {
		/* check if there are no pending items */
		if (vortex_async_queue_items (ctx->log_manager_queue) == 0)
			return axl_true;

		/* get descriptor */
		log_descriptor = vortex_async_queue_pop (ctx->log_manager_queue);
			
		/* check item received: if null received terminate loop */
		if (PTR_TO_INT (log_descriptor) == -4)
			return axl_false;

		/* register log_descriptor on the list */
		axl_list_append (ctx->log_manager_list, log_descriptor);       
	} /* end if */

	return axl_true;
}

void __turbulence_log_manager_transfer_content (TurbulenceCtx * ctx)
{
	int                       size;
	int                       size_written;
	char                      buffer[4097];
	TurbulenceLogDescriptor * log_descriptor;

	/* reset cursor */
	axl_list_cursor_first (ctx->log_manager_cursor);
	while (axl_list_cursor_has_item (ctx->log_manager_cursor)) {
		/* get log descriptor */
		log_descriptor = axl_list_cursor_get (ctx->log_manager_cursor);

		/* check if the log descriptor is set */
		if (__vortex_io_waiting_default_is_set (log_descriptor->descriptor, ctx->log_manager_fileset, NULL)) {
			
			/* read content */
			size = read (log_descriptor->descriptor, buffer, 4096);
			
			/* check closed socket (child process finished) */
			if (size <= 0) {
				/* remove descriptor link */
				axl_list_cursor_remove (ctx->log_manager_cursor); 

				continue;
			}
			
			/* transfer content to the associated socket */
			buffer[size] = 0;
			size_written = write (log_descriptor->output_sink, buffer, size);
			if (size_written != size) {
				error ("failed to write log received from child, content differs (%d != %d), error was: %s", 
				       size, size_written,
				       vortex_errno_get_last_error ());
			} /* end if */
		} /* end if */
		
		/* get the next item */
		axl_list_cursor_next (ctx->log_manager_cursor);
	} /* end if */

	return;
}

axlPointer __turbulence_log_manager_run (TurbulenceCtx * ctx)
{
	int                       max_fds;
	int                       result;

	/* init here list, its cursor, the fileset to watch fd for
	 * changes and a queue to receive new registrations */
	ctx->log_manager_list    = axl_list_new (axl_list_always_return_1, __turbulence_log_manager_descriptor_free);
	ctx->log_manager_cursor  = axl_list_cursor_new (ctx->log_manager_list);
	/* force to use always default select(2) based implementation */
	ctx->log_manager_fileset = __vortex_io_waiting_default_create (turbulence_ctx_get_vortex_ctx (ctx), READ_OPERATIONS);
	ctx->log_manager_queue   = vortex_async_queue_new ();

	/* now loop watching content from the list */
wait_for_first_item:
	if (! __turbulence_log_manager_read_first (ctx))
		return NULL;
	
	while (axl_true) {
		/* build file set to watch */
		max_fds = __turbulence_log_build_watch_set (ctx);

		/* check if no descriptor must be watch */
		if (axl_list_length (ctx->log_manager_list) == 0) {
			msg ("no more log descriptors found to be watched, putting thread to sleep");
			goto wait_for_first_item;
		} /* end if */
		
		/* perform IO wait operation */
		result = __vortex_io_waiting_default_wait_on (ctx->log_manager_fileset, max_fds, READ_OPERATIONS);
		
		/* check for timeout and errors */
		if (result == -1 || result == -2)
			goto process_pending;
		if (result == -3) {
			error ("fatal error received from io-wait function, finishing turbulence log manager..");
			return NULL;
		} /* end if */

		/* transfer content found */
		if (result > 0) 
			__turbulence_log_manager_transfer_content (ctx);

	process_pending:
		/* check for pending descriptors and stop the loop if
		 * found a signal for this */
		if (! __turbulence_log_manager_read_pending (ctx))
			return NULL;
	} /* end if */
	

	return NULL;
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
	if (! vortex_thread_create (&ctx->log_manager_thread, 
				    (VortexThreadFunc) __turbulence_log_manager_run,
				    ctx,
				    VORTEX_THREAD_CONF_END)) {
		error ("unable to start log manager loop, checking clean start..");
		CLEAN_START (ctx);
		return;
	} /* end if */
	
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
	TurbulenceLogDescriptor * log_descriptor;

	v_return_if_fail (ctx);

	/* create log descriptor linking the descriptor received to
	 * the current descriptor managing the context signaled by
	 * type */
	log_descriptor = axl_new (TurbulenceLogDescriptor, 1);
	log_descriptor->descriptor = descriptor;
	msg ("register fd: %d (type: %d)", descriptor, type);
	switch (type) {
	case LOG_REPORT_GENERAL:
		/* configure general log watcher */
		log_descriptor->output_sink = ctx->general_log;
		break;
	case LOG_REPORT_ERROR:
		/* configure error log watcher */
		log_descriptor->output_sink = ctx->error_log;
		break;
	case LOG_REPORT_ACCESS:
		/* configure access log watcher */
		log_descriptor->output_sink = ctx->access_log;
		break;
	case LOG_REPORT_VORTEX:
		/* configure vortex log watcher */
		log_descriptor->output_sink = ctx->vortex_log;
		break;
	}
	
	/* push new log descriptor */
	vortex_async_queue_push (ctx->log_manager_queue, log_descriptor);
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
	time_str [strlen (time_str) - 1] = 0;

	/* write stamp */
	string = axl_strdup_printf ("%s [%d] (%s:%d) ", time_str, getpid (), file, line);
	length = strlen (string);
	axl_free (time_str);

	/* create message */
	string2 = axl_strdup_printfv (message, args);
	length2 = strlen (string2);

	/* build final log message */
	total  = length + length2 + 1;
	result = axl_new (char, total + 1);
	memcpy (result, string, length);
	memcpy (result + length, string2, length2);
	memcpy (result + length + length2, "\n", 1);
	
	/* write content: do it in a single operation to avoid mixing
	 * content from different logs at the log file. */
	write (log, result, total);
	
	/* release memory used */
	axl_free (result);
	axl_free (string);
	axl_free (string2);
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

/** 
 * @internal
 * @brief Stops and dealloc all resources hold by the module.
 */
void turbulence_log_cleanup (TurbulenceCtx * ctx)
{
	/* close the general log */
	if (ctx->general_log)
		close (ctx->general_log);
	ctx->general_log = -1;

	/* close the error log */
	if (ctx->error_log)
		close (ctx->error_log);
	ctx->error_log = -1;

	/* close the access log */
	if (ctx->access_log)
		close (ctx->access_log);
	ctx->access_log = -1;

	/* close vortex log */
	if (ctx->vortex_log)
		close (ctx->vortex_log);
	ctx->vortex_log = -1;

	/* now finish log manager */
	if (ctx->log_manager_queue != NULL) {
		vortex_async_queue_push (ctx->log_manager_queue, INT_TO_PTR (-4));
		vortex_thread_destroy (&ctx->log_manager_thread, axl_false);
		
		/* free list, queue and fileset */
		turbulence_log_child_cleanup (ctx);
	} /* end if */

	return;
}

/** 
 * @internal Function that release object used by central logging but
 * also cleanups all elements not required by child processes.
 */
void      turbulence_log_child_cleanup (TurbulenceCtx * ctx)
{
	v_return_if_fail (ctx);

	axl_list_free (ctx->log_manager_list);
	ctx->log_manager_list = NULL;

	axl_list_cursor_free (ctx->log_manager_cursor);
	ctx->log_manager_cursor = NULL;

	vortex_async_queue_unref (ctx->log_manager_queue);
	ctx->log_manager_queue = NULL;

	__vortex_io_waiting_default_destroy (ctx->log_manager_fileset);
	ctx->log_manager_fileset = NULL;

	return;
}


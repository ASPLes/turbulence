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
	ctx->general_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND);
	if (ctx->general_log == -1) {
		abort_error ("unable to open general log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open error logs */
	node      = axl_node_get_child_called (node, "error-log");
	ctx->error_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND);
	if (ctx->error_log == -1) {
		abort_error ("unable to open error log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open access log */
	node      = axl_node_get_child_called (node, "access-log");
	ctx->access_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND);
	if (ctx->access_log == -1) {
		abort_error ("unable to open access log: %s", ATTR_VALUE (node, "file"));
		CLEAN_START (ctx);
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	node      = axl_node_get_child_called (node, "vortex-log");
	ctx->vortex_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND);
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
 * @internal Handler used to check channel creation against an
 * internal password.
 */
axl_bool  turbulence_log_bridge_start (const char        * profile,
				       int                 channel_num,
				       VortexConnection  * connection,
				       const char        * serverName,
				       const char        * profile_content,
				       char             ** profile_content_reply,
				       VortexEncoding      encoding,
				       axlPointer          user_data)
{
	TurbulenceCtx * ctx = (TurbulenceCtx *) user_data;

	/* check pass provided by user */
	if (axl_cmp (ctx->log_bridge_pass, profile_content)) {
		msg ("log bridget accepted");
		return axl_true;
	}
	error ("unable to accept log bridge channel, random pass do not match!");
	return axl_false;
}

void turbulence_log_bridge_frame_received (VortexChannel    * channel,
					   VortexConnection * conn,
					   VortexFrame      * frame,
					   axlPointer         user_data)
{
	TurbulenceCtx * ctx = (TurbulenceCtx *) user_data;

	/* reply to frame received */
	vortex_channel_send_rpy (channel, "", 0, vortex_frame_get_msgno (frame));
	
	/* received frame: write the corresponding message */
	if (axl_memcmp ("general-log", vortex_frame_get_payload (frame), 11)) {
		msg2 (vortex_frame_get_payload (frame) + 4);
	} else if (axl_memcmp ("msg", vortex_frame_get_payload (frame), 3))
		msg (vortex_frame_get_payload (frame) + 3);
	else if (axl_memcmp ("error", vortex_frame_get_payload (frame), 4))
		msg (vortex_frame_get_payload (frame) + 4);
	else if (axl_memcmp ("wrn", vortex_frame_get_payload (frame), 3))
		msg (vortex_frame_get_payload (frame) + 3);
	return;
}

#if defined(DEFINE_RANDOM_PROTO)
long int random (void);
#endif

/** 
 * @internal Function used to init and register turbulence log
 * bridging: the facility used to register logs from turbulence child
 * process.
 */
void turbulence_log_bridge_init (TurbulenceCtx * ctx)
{
	/* init bridge random pass: FIXME: we should store the random
	 * and recover it to avoid using always as seed 1 */
	long int   rand_value            = random ();

	/* set bridge pass */
	ctx->log_bridge_pass  = axl_strdup_printf ("%ld", rand_value);

	msg ("configured log bridge random password: %s", ctx->log_bridge_pass);

	vortex_profiles_register (turbulence_ctx_get_vortex_ctx (ctx), 
				  "urn:aspl.es:beep:profiles:turbulence-log-bridge",
				  /* no start handler */
				  NULL, NULL, 
				  /* no close */
				  NULL, NULL,
				  /* frame received */
				  turbulence_log_bridge_frame_received, ctx);

	/* register extended start to get access to the piggy back */
	vortex_profiles_register_extended_start (turbulence_ctx_get_vortex_ctx (ctx), 
						 "urn:aspl.es:beep:profiles:turbulence-log-bridge",
						 turbulence_log_bridge_start, ctx);
	
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

	/* create timestamp */
	time_val = time (NULL);
	time_str = axl_strdup (ctime (&time_val));
	time_str [strlen (time_str) - 1] = 0;

	/* write stamp */
	string = axl_strdup_printf ("%s (%s:%d) ", time_str, file, line);
	write (log, string, strlen (string));
	axl_free (string);
	axl_free (time_str);

	/* create message */
	string = axl_strdup_printfv (message, args);

	/* write message */
	write (log, string, strlen (string));
	axl_free (string);
	write  (log, "\n", 1);
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

	return;
}


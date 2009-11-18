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
#include <turbulence-process.h>

/* include private headers */
#include <turbulence-ctx-private.h>

/* include signal handler: SIGCHLD */
#include <signal.h>

/** 
 * @internal Function used to init process module (for its internal
 * handling).
 *
 * @param ctx The turbulence context where the initialization will
 * take place.
 */
void turbulence_process_init         (TurbulenceCtx * ctx, axl_bool reinit)
{
	/* check for reinit operation */
	if (reinit && ctx->child_process) {
		axl_list_free (ctx->child_process);
		ctx->child_process = NULL;
	}

	/* create the list of childs opened */
	if (ctx->child_process == NULL)
		ctx->child_process = axl_list_new (axl_list_equal_int, NULL);
	/* init mutex */
	vortex_mutex_create (&ctx->child_process_mutex);
	return;
}

void turbulence_process_finished (VortexCtx * ctx, axlPointer user_data)
{
	/* unlock waiting child */
	vortex_listener_unlock (ctx);
	return;
}

/** 
 * @internal Reference to the context used by the child process inside
 * turbulence_process_create_child.
 */
TurbulenceCtx * child_ctx   = NULL;

void turbulence_process_signal_received (int _signal) {
	/* default handling */
	turbulence_signal_received (child_ctx, _signal);
	
	return;
}


/** 
 * @internal Allows to create a child process running listener connection
 * provided.
 */
void turbulence_process_create_child (TurbulenceCtx       * ctx, 
				      VortexConnection    * conn, 
				      TurbulencePPathDef  * def,
				      axl_bool              handle_start_reply,
				      int                   channel_num,
				      const char          * profile,
				      const char          * profile_content,
				      VortexEncoding        encoding,
				      const char          * serverName,
				      VortexFrame         * frame)
{
	int                   pid;
	VortexCtx        *    vortex_ctx;
	VortexChannel    *    channel0;

	/* pipes to communicate logs from child to parent */
	int                   general_log[2] = {-1, -1};
	int                   error_log[2]   = {-1, -1};
	int                   access_log[2]  = {-1, -1};
	int                   vortex_log[2]  = {-1, -1};

	if (turbulence_log_is_enabled (ctx)) {
		if (pipe (general_log) != 0)
			error ("unable to create pipe to transport general log, this will cause these logs to be lost");
		if (pipe (error_log) != 0)
			error ("unable to create pipe to transport error log, this will cause these logs to be lost");
		if (pipe (access_log) != 0)
			error ("unable to create pipe to transport access log, this will cause these logs to be lost");
		if (pipe (vortex_log) != 0)
			error ("unable to create pipe to transport vortex log, this will cause these logs to be lost");
	} /* end if */

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		/* unwatch the connection from the parent to avoid
		   receiving more content which now handled by the
		   child */
		vortex_reader_unwatch_connection (CONN_CTX (conn), conn);

		/* register pipes to receive child logs */
		if (turbulence_log_is_enabled (ctx)) {
			/* support for general-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_GENERAL, general_log[0]); /* register read end */
			vortex_close_socket (general_log[1]);                                      /* close write end */

			/* support for error-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_ERROR, error_log[0]); /* register read end */
			vortex_close_socket (error_log[1]);                                      /* close write end */

			/* support for access-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_ACCESS, access_log[0]); /* register read end */
			vortex_close_socket (access_log[1]);                                      /* close write end */

			/* support for vortex-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_VORTEX, vortex_log[0]); /* register read end */
			vortex_close_socket (vortex_log[1]);                                      /* close write end */
		} /* end if */

		/* register the child process identifier */
		vortex_mutex_lock (&ctx->child_process_mutex);
		axl_list_append (ctx->child_process, INT_TO_PTR (pid));
		vortex_mutex_unlock (&ctx->child_process_mutex);

		/* record child */
		msg ("PARENT=%d: Created child process pid=%d (childs: %d)", getpid (), pid, turbulence_process_child_count (ctx));
		return;
	} /* end if */

	/* do not log messages until turbulence_ctx_reinit finishes */
	child_ctx = ctx;

	/* reinit TurbulenceCtx */
	turbulence_ctx_reinit (ctx);

	/* check if log is enabled to redirect content to parent */
	if (turbulence_log_is_enabled (ctx)) {
		/* configure new general log */
		turbulence_log_configure (ctx, LOG_REPORT_GENERAL, general_log[1]); /* configure write end */
		vortex_close_socket (general_log[0]);                               /* close read end */

		/* configure new error log */
		turbulence_log_configure (ctx, LOG_REPORT_ERROR, error_log[1]); /* configure write end */
		vortex_close_socket (error_log[0]);                             /* close read end */

		/* configure new access log */
		turbulence_log_configure (ctx, LOG_REPORT_ACCESS, access_log[1]); /* configure write end */
		vortex_close_socket (access_log[0]);                              /* close read end */

		/* configure new vortex log */
		turbulence_log_configure (ctx, LOG_REPORT_VORTEX, vortex_log[1]); /* configure write end */
		vortex_close_socket (vortex_log[0]);                              /* close read end */
	}

	/* cleanup log stuff used only by the parent process */
	turbulence_loop_close (ctx->log_manager, axl_false);
	ctx->log_manager = NULL;

	/* reconfigure signals */
	turbulence_signal_install (ctx, 
				   /* disable sigint */
				   axl_false, 
				   /* signal sighup */
				   axl_false,
				   /* enable sigchild */
				   axl_false,
				   turbulence_process_signal_received);

	msg ("CHILD: Created child process: %d", getpid ());

	/* check here to change root path, in the case it is defined
	 * now we still have priviledges */
	turbulence_ppath_change_root (ctx, def);

	/* check here for setuid support */
	turbulence_ppath_change_user_id (ctx, def);

	/* reinit vortex ctx */
	vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);
	vortex_ctx_reinit (vortex_ctx);

	/* call to unload modules after fork */
	turbulence_module_unload_after_fork (ctx);

	/* set finish handler that will help to finish child process
	   (or not) */
	vortex_ctx_set_on_finish (vortex_ctx, turbulence_process_finished, NULL);

	/* (re)start vortex */
	if (! vortex_init_ctx (vortex_ctx)) {
		error ("failed to restart vortex engine after fork operation");
		exit (-1);
	} /* end if */

	/* now finish and register the connection */
	vortex_connection_set_close_socket (conn, axl_true);
	vortex_reader_watch_connection (vortex_ctx, conn);

	/* check to handle start reply message */
	if (handle_start_reply) {
		/* handle start channel reply */
		if (! vortex_channel_0_handle_start_msg_reply (vortex_ctx, conn, channel_num,
							       profile, profile_content,
							       encoding, serverName, frame)) {
			error ("Channel start not accepted on child process, finising process=%d, closing conn id=%d..",
			       getpid (), vortex_connection_get_id (conn));

			/* wait here so the error message reaches the
			 * remote BEEP peer */
			channel0 = vortex_connection_get_channel (conn, 0);
			vortex_channel_block_until_replies_are_sent (channel0, 1000);
		}
		msg ("Channel start accepted on child..");
	} /* end if */
	
	msg ("child process created...wait for exit");
	vortex_listener_wait (turbulence_ctx_get_vortex_ctx (ctx));
	msg ("finishing process...");

	/* release frame received */
	vortex_frame_unref (frame);

	/* terminate turbulence execution */
	turbulence_exit (ctx, axl_false, axl_false);

	/* free context (the very last operation) */
	turbulence_ctx_free (ctx);
	vortex_ctx_free (vortex_ctx);

	/* finish process */
	exit (0);
	
	return;
}

#if defined(DEFINE_KILL_PROTO)
int kill (int pid, int signal);
#endif

/** 
 * @internal Function that allows to check and kill childs started by
 * turbulence acording to user configuration.
 *
 * @param ctx The context where the child stop operation will take
 * place.
 */ 
void turbulence_process_kill_childs  (TurbulenceCtx * ctx)
{
	axlDoc  * doc;
	axlNode * node;
	int       pid;
	int       iterator;

	/* get user doc */
	doc  = turbulence_config_get (ctx);
	node = axl_doc_get (ctx->config, "/turbulence/global-settings/kill-childs-on-exit");
	if (node == NULL) {
		error ("Unable to find kill-childs-on-exit node, doing nothing..");
		return;
	}

	/* check if we have to kill childs */
	if (! HAS_ATTR_VALUE (node, "value", "yes")) {
		error ("leaving childs running (kill-childs-on-exit not enabled)..");
		return;
	} /* end if */

	/* send a kill operation to all childs */
	vortex_mutex_lock (&ctx->child_process_mutex);
	iterator = 0;
	while (iterator < axl_list_length (ctx->child_process)) {
		/* get pid */
		pid = PTR_TO_INT (axl_list_get_nth (ctx->child_process, iterator));
		msg ("killing child process: %d", pid);

		/* send term signal */
		if (kill (pid, SIGTERM) != 0)
			error ("failed to kill child (%d) error was: %d:%s",
			       pid, errno, vortex_errno_get_last_error ());
		

		/* next iterator */
		iterator++;
	} /* end while */
	vortex_mutex_unlock (&ctx->child_process_mutex);

	return;
}

/** 
 * @brief Allows to return the number of child processes  created. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @return Number of child process created or -1 it if fails.
 */
int      turbulence_process_child_count  (TurbulenceCtx * ctx)
{
	int count;

	/* check context received */
	if (ctx == NULL)
		return -1;

	vortex_mutex_lock (&ctx->child_process_mutex);
	count = axl_list_length (ctx->child_process);
	vortex_mutex_unlock (&ctx->child_process_mutex);
	msg ("child process count: %d..", count);
	return count;
}

/** 
 * @brief Allows to check if the provided process Id belongs to a
 * child process currently running.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param pid The process identifier.
 *
 * @return axl_true if the process exists, otherwise axl_false is
 * returned.
 */
axl_bool turbulence_process_child_exits  (TurbulenceCtx * ctx, int pid)
{
	axl_bool result;

	/* check context received */
	if (ctx == NULL || pid < 0)
		return axl_false;

	vortex_mutex_lock (&ctx->child_process_mutex);
	result = axl_list_exists (ctx->child_process, INT_TO_PTR (pid));
	vortex_mutex_unlock (&ctx->child_process_mutex);

	return result;
}

/** 
 * @internal Function used to cleanup the process module.
 */
void turbulence_process_cleanup      (TurbulenceCtx * ctx)
{
	vortex_mutex_destroy (&ctx->child_process_mutex);
	axl_list_free (ctx->child_process);
	return;
			      
}

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
TurbulenceCtx * ctx       = NULL;

void turbulence_process_signal_received (int _signal) {
	/* default handling */
	int pid = turbulence_signal_received (ctx, _signal);
	
	if (_signal == SIGCHLD) {
		msg ("child process finished, removing from child list: %d", pid);
		/* remove pid from list */
		vortex_mutex_lock (&ctx->child_process_mutex);
		
		axl_list_remove (ctx->child_process, INT_TO_PTR (pid));

		vortex_mutex_unlock (&ctx->child_process_mutex);
	} /* end if */

	return;
}


/** 
 * @internal Allows to create a child process running listener connection
 * provided.
 */
void turbulence_process_create_child (TurbulenceCtx       * _ctx, 
				      VortexConnection    * conn, 
				      TurbulencePPathDef  * def)
{
	int                   pid;
	VortexCtx        *    vortex_ctx;

	msg ("Creating child process to manage connection id=%d", vortex_connection_get_id (conn));

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		/* parent code, just return */
		vortex_connection_set_close_socket (conn, axl_false);
		vortex_connection_shutdown (conn); 

		/* record child */
		msg ("Created child process pid=%d", pid);
		vortex_mutex_lock (&ctx->child_process_mutex);
		axl_list_append (ctx->child_process, INT_TO_PTR (pid));
		vortex_mutex_unlock (&ctx->child_process_mutex);

		return;
	} /* end if */

	/* do not log messages until turbulence_ctx_reinit finishes */
	ctx = _ctx;
	
	/* reinit TurbulenceCtx */
	turbulence_ctx_reinit (ctx);

	/* reconfigure signals */
	turbulence_signal_install (ctx, 
				   /* disable sigint */
				   axl_false, 
				   /* signal sighup */
				   axl_false,
				   /* enable sigchild */
				   axl_false,
				   turbulence_process_signal_received);

	msg ("Created child prcess: %d", getpid ());

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

	/* set finish handler  */
	vortex_ctx_set_on_finish (vortex_ctx, turbulence_process_finished, NULL);

	/* restart vortex */
	if (! vortex_init_ctx (vortex_ctx)) {
		error ("failed to restart vortex engine after fork operation");
		exit (-1);
	} /* end if */

	/* now finish and register the connection */
	vortex_listener_complete_register (conn, axl_true);
	
	msg ("child process created...wait for exit");
	vortex_listener_wait (turbulence_ctx_get_vortex_ctx (ctx));
	msg ("finishing process...");

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

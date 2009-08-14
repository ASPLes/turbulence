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

void turbulence_process_finished (VortexCtx * ctx, axlPointer user_data)
{
	VortexAsyncQueue * lock = (VortexAsyncQueue *) user_data;
	/* found vortex reader finished, lock it */
	vortex_async_queue_push (lock, INT_TO_PTR (axl_true));
	return;
}

/** 
 * @internal Reference to the context used by the child process inside
 * turbulence_process_create_child.
 */
TurbulenceCtx * ctx       = NULL;

void turbulence_process_signal_received (int _signal) {
	/* default handling */
	turbulence_signal_received (ctx, _signal);
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
	VortexAsyncQueue *    lock;
	VortexCtx        *    vortex_ctx;

	msg ("Creating child process to manage connection id=%d", vortex_connection_get_id (conn));

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		/* parent code, just return */
		vortex_connection_set_close_socket (conn, axl_false);
		vortex_connection_shutdown (conn); 
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

	/* set finish handler  */
	lock = vortex_async_queue_new ();
	vortex_ctx_set_on_finish (vortex_ctx, turbulence_process_finished, lock);

	/* restart vortex */
	if (! vortex_init_ctx (vortex_ctx)) {
		error ("failed to restart vortex engine after fork operation");
		exit (-1);
	} /* end if */

	/* now finish and register the connection */
	vortex_listener_complete_register (conn, axl_true);
	
	msg ("child process created...wait for exit");
	vortex_async_queue_pop (lock);
	msg ("finishing process...");
	vortex_async_queue_unref (lock);

	/* terminate turbulence execution */
	turbulence_exit (ctx, axl_false, axl_false);

	/* free context (the very last operation) */
	turbulence_ctx_free (ctx);
	vortex_ctx_free (vortex_ctx);
	
	/* finish process */
	exit (0);
	
	return;
}



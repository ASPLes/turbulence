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
#include <turbulence-ctx-private.h>
#include <signal.h>
#include <sys/wait.h>

/** 
 * @internal Termination signal received, notify.
 * @param _signal The signal received.
 */
void turbulence_signal_received (TurbulenceCtx * ctx, int _signal)
{
	int exit_status = 0;
	int pid;
	if (_signal == SIGHUP) {
		msg ("received reconf signal, handling..");
		/* notify */
		turbulence_reload_config (ctx, _signal);

#if defined(AXL_OS_UNIX)
		/* reconfigure signal */
		signal (SIGHUP, ctx->signal_handler);
#endif
		return;
	} else if (_signal == SIGCHLD) {
		pid = wait (&exit_status);
		msg ("child process (%d) finished with status: %d",
		     pid, exit_status);

		/* reconfigure signal */
		signal (SIGHUP, ctx->signal_handler);
		return;
	} /* end if */

	/* notify */
	turbulence_signal_exit (ctx, _signal);

	return;	
}

/** 
 * @brief Allows to install default signal handling.
 */
void turbulence_signal_install (TurbulenceCtx           * ctx, 
				axl_bool                  enable_sigint, 
				axl_bool                  enable_sighup,
				axl_bool                  enable_sigchild,
				TurbulenceSignalHandler   signal_handler)
{
	/* install default handlers */
	/* check for sigint */
	if (enable_sigint)
		signal (SIGINT,  signal_handler); 		
	signal (SIGSEGV, signal_handler);
	signal (SIGABRT, signal_handler);
	signal (SIGTERM, signal_handler); 

	/* check for sigchild */
	if (enable_sigchild)
		signal (SIGCHLD, signal_handler);

#if defined(AXL_OS_UNIX)
	signal (SIGKILL, signal_handler);
	signal (SIGQUIT, signal_handler);

	/* check for sighup */
	if (enable_sighup)
		signal (SIGHUP,  signal_handler);
#endif

	/* configure handlers received */
	ctx->signal_handler = signal_handler;

	return;
}

/** 
 * @brief Terminates the turbulence excution, returing the exit signal
 * provided as first parameter. This function is used to notify a
 * context that a signal was received.
 * 
 * @param signal The exit code to return.
 */
void turbulence_signal_exit (TurbulenceCtx * ctx, int _signal)
{
	/* get turbulence context */
	axlDoc           * doc;
	axlNode          * node;
	VortexAsyncQueue * queue;
	VortexCtx        * vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	/* lock the mutex and check */
	vortex_mutex_lock (&ctx->exit_mutex);
	if (ctx->is_existing) {
		/* other thread is already cleaning */
		vortex_mutex_unlock (&ctx->exit_mutex);
		return;
	} /* end if */

	/* flag that turbulence is existing and do all cleanup
	 * operations */
	ctx->is_existing = true;
	vortex_mutex_unlock (&ctx->exit_mutex);
	
	switch (_signal) {
	case SIGINT:
		msg ("caught SIGINT, terminating turbulence..");
		break;
	case SIGTERM:
		msg ("caught SIGTERM, terminating turbulence..");
		break;
#if defined(AXL_OS_UNIX)
	case SIGKILL:
		msg ("caught SIGKILL, terminating turbulence..");
		break;
	case SIGQUIT:
		msg ("caught SIGQUIT, terminating turbulence..");
		break;
#endif
	case SIGSEGV:
	case SIGABRT:
		error ("caught %s, anomalous termination (this is an internal turbulence or module error)",
		       _signal == SIGSEGV ? "SIGSEGV" : "SIGABRT");
		
		/* check current termination option */
		doc  = turbulence_config_get (ctx);
		node = axl_doc_get (doc, "/turbulence/global-settings/on-bad-signal");
		if (HAS_ATTR_VALUE (node, "action", "ignore")) {
			/* ignore the signal emision */
			return;
		} else if (HAS_ATTR_VALUE (node, "action", "hold")) {
			/* lock the process */
			error ("Bad signal found, locking process, now you can attach or terminate pid: %d", 
			       getpid ());
			queue = vortex_async_queue_new ();
			vortex_async_queue_pop (queue);
			return;
		}
		
		/* let turbulence to exit */
		break;
	default:
		msg ("terminating turbulence..");
		break;
	} /* end if */

	/* Unlock the listener here. Do not perform any deallocation
	 * operation here because we are in the middle of a signal
	 * handler execution. By unlocking the listener, the
	 * turbulence_cleanup is called cleaning the room. */
	vortex_listener_unlock (vortex_ctx);

	return;
} /* end if */







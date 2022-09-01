/*  Turbulence BEEP application server
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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
#include <turbulence-ctx-private.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

/** 
 * \defgroup turbulence_signal Turbulence Signal : signal handling support for turbulence
 */

/** 
 * \addtogroup turbulence_signal
 * @{
 */

/** 
 * @brief Signal notify facility. This function is used to signal on
 * the appropriate \ref TurbulenceCtx, a particular signal received.
 *
 * @param ctx The turbulence context where the signal will be handled.
 * @param _signal The signal received.
 *
 * @return Returns 0 or the pid in the case the signal is SIGHLD.
 */
int turbulence_signal_received (TurbulenceCtx * ctx, int _signal)
{
	int exit_status = 0;
	int pid;
	TurbulenceChild * child;

	if (_signal == SIGHUP) {
		msg ("received reconf signal, handling..");
		/* notify */
		turbulence_reload_config (ctx, _signal);

#if defined(AXL_OS_UNIX)
		/* reconfigure signal */
		signal (SIGHUP, ctx->signal_handler);
#endif
		return 0;
	} else if (_signal == SIGCHLD) {
		/* do not get finished pid to let kill child process
		 * to get it */
		pid = wait (&exit_status);
		msg ("child process (%d) finished with status: %d",
		     pid, exit_status);

		/* lock to remove */
		vortex_mutex_lock (&ctx->child_process_mutex);

		/* get child to reduce childs running */
		child = axl_hash_get (ctx->child_process, INT_TO_PTR (pid));
		if (child && child->ppath) {
			/* decrease number of childs running */
			child->ppath->childs_running--;
		} /* end if */

		/* remove pid from list */
		axl_hash_remove (ctx->child_process, INT_TO_PTR (pid));

		/* unlock */
		vortex_mutex_unlock (&ctx->child_process_mutex);

		/* reconfigure signal again */
		signal (SIGCHLD, ctx->signal_handler);
		
		/* return child pid to allow management */
		return pid;
	} /* end if */

	/* notify */
	turbulence_signal_exit (ctx, _signal);

	return 0;	
}


/** 
 * @brief Allows to enable sigchild handling.
 */
void turbulence_signal_sigchld (TurbulenceCtx * ctx, axl_bool enable)
{
	msg ("Enabling SIGCHLD handling at turbulence main process");

	/* check for sigchild */
	if (enable)
		signal (SIGCHLD, ctx->signal_handler);
	else
		signal (SIGCHLD, NULL);	

	return;
}


/** 
 * @brief Allows to install default signal handling.
 */
void turbulence_signal_install (TurbulenceCtx           * ctx, 
				axl_bool                  enable_sigint, 
				axl_bool                  enable_sighup,
				TurbulenceSignalHandler   signal_handler)
{
	/* install default handlers */
	/* check for sigint */
	if (enable_sigint)
		signal (SIGINT,  signal_handler); 		
	else 
		signal (SIGINT,  NULL); 		
	signal (SIGSEGV, signal_handler);
	signal (SIGABRT, signal_handler);
	signal (SIGTERM, signal_handler); 

#if defined(AXL_OS_UNIX)
/*	signal (SIGKILL, signal_handler); */
	signal (SIGQUIT, signal_handler);

	/* check for sighup */
	if (enable_sighup)
		signal (SIGHUP,  signal_handler);
	else
		signal (SIGHUP, NULL);
#endif

	/* configure handlers received */
	ctx->signal_handler = signal_handler;

	return;
}

#define CHECK_AND_REPORT_MAIL_TO(subject, body, file) do{		           \
	if (HAS_ATTR (node, "mail-to")) {				           \
		turbulence_support_simple_smtp_send (ctx,		           \
						     ATTR_VALUE (node, "mail-to"), \
						     subject, body, file);         \
	} /* end if */							           \
} while (0)

/** 
 * @internal Common implementation.
 */
axl_bool __turbulence_signal_common_block (TurbulenceCtx * ctx,
					   int             signal,
					   axl_bool        block_signal)
{
	sigset_t     intmask;
	const char * label = block_signal ? "blocking" : "unblocking";

	/* msg ("Starting %s of signal %d", label, signal); */

	sigemptyset (&intmask);
	if ((sigemptyset(&intmask) == -1) || (sigaddset(&intmask, SIGCHLD) == -1)){  
		error ("Failed to initialize the signal=%d mask", signal);
		return axl_false;
	}

	if (sigprocmask (block_signal ? SIG_BLOCK : SIG_UNBLOCK, &intmask, NULL) == -1) {
		error ("Failed while %s signal=%d, errno=%d:%s", label, signal, errno, vortex_errno_get_last_error ());
		return axl_false;
	}

	return axl_true;
}

/** 
 * @brief Allows to block the provided signal delaying its delivery.
 *
 * @param ctx The turbulence context.
 * @param signal The signal to block.
 *
 * @return axl_true if signal was blocked, otherwise axl_false is
 * returned.
 */
axl_bool turbulence_signal_block   (TurbulenceCtx * ctx,
				    int             signal)
{
	/* call to block */
	return __turbulence_signal_common_block (ctx, signal, axl_true);
}

/** 
 * @brief Allows to unblock the provided signal delaying its delivery.
 *
 * @param ctx The turbulence context.
 * @param signal The signal to unblock.
 *
 * @return axl_true if signal was unblocked, otherwise axl_false is
 * returned.
 */
axl_bool turbulence_signal_unblock (TurbulenceCtx * ctx,
				    int             signal)
{
	/* call to unblock */
	return __turbulence_signal_common_block (ctx, signal, axl_false);
}

/** 
 * @internal Terminates the turbulence excution, returing the exit signal
 * provided as first parameter. This function is used to notify a
 * context that a signal was received.
 * 
 * @param ctx The turbulence context to terminate.
 * @param _signal The exit code to return.
 */
void turbulence_signal_exit (TurbulenceCtx * ctx, int _signal)
{
	/* get turbulence context */
	axlDoc           * doc;
	axlNode          * node;
	VortexAsyncQueue * queue;
	char             * backtrace_file;

	/* lock the mutex and check */
	vortex_mutex_lock (&ctx->exit_mutex);
	if (ctx->is_exiting) {
		msg ("process already existing, signal received=%d, doing nothing...", _signal);

		/* other thread is already cleaning */
		vortex_mutex_unlock (&ctx->exit_mutex);
		return;
	} /* end if */

	msg ("received termination signal (%d) on PID %d, preparing exit process", _signal, getpid ()); 

	/* flag that turbulence is existing and do all cleanup
	 * operations */
	ctx->is_exiting = axl_true;
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
		error ("applying configured action %s", (node && HAS_ATTR (node, "action")) ? ATTR_VALUE (node, "action") : "not defined");
		if (HAS_ATTR_VALUE (node, "action", "ignore")) {
			/* do notify if enabled */
			CHECK_AND_REPORT_MAIL_TO ("Bad signal received at turbulence process, default action: ignore",
						  "Received termination signal but it was ignored.",
						  NULL);

			/* ignore the signal emision */
			return;
		} else if (HAS_ATTR_VALUE (node, "action", "hold")) {
			/* lock the process */
			error ("Bad signal found, locking process, now you can attach or terminate pid: %d", 
			       getpid ());
			CHECK_AND_REPORT_MAIL_TO ("Bad signal received a turbulence process, default action: hold",
						  "Received termination signal and the process was hold for examination",
						  NULL);
			queue = vortex_async_queue_new ();
			vortex_async_queue_pop (queue);
			return;
		} else if (HAS_ATTR_VALUE (node, "action", "backtrace")) {
			/* create temporal file */
			error ("Bad signal found, creating backtrace for current process: %d", getpid ());
			backtrace_file = turbulence_support_get_backtrace (ctx, getpid ());
			if (backtrace_file == NULL)
				error ("..backtrace error, unable to produce backtrace..");
			else
				error ("..backtrace created at: %s", backtrace_file);

			/* check if we have to do a mail notification */
			CHECK_AND_REPORT_MAIL_TO ("Bad signal received a turbulence process, default action: backtrace",
						  NULL, backtrace_file);
			/* release backtrace */
			axl_free (backtrace_file);
		}

		/* signal vortex to not terminate threads (to avoid
		 * deadlocks) */
		exit (-1);
		break;
	default:
		msg ("terminating turbulence..");
		break;
	} /* end if */

	/* Unlock the listener here. Do not perform any deallocation
	 * operation here because we are in the middle of a signal
	 * handler execution. By unlocking the listener, the
	 * turbulence_cleanup is called cleaning the room. */
	msg ("Unlocking turbulence listener: %p", ctx);
	/* unlock the current listener */
	vortex_listener_unlock (TBC_VORTEX_CTX (ctx));

	return;
} /* end if */

/** 
 * @}
 */





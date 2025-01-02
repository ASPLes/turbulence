/*  Turbulence BEEP application server
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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

/* system includes */
#include <signal.h>

/* include common definitions */
#include <main-common.h>

/* local private header */
#include <turbulence-ctx-private.h>

/* global instance created */
TurbulenceCtx * ctx;

/** 
 * @internal Init for all exarg functions provided by the turbulence
 * command line.
 */
int  main_init_exarg (int argc, char ** argv)
{
	/* set starting process name */
	turbulence_process_set_file_path (argv[0]);

	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install default debug options. */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Show turbulence version.");

	/* install default debug options. */
	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Makes all log produced by the application, to be also dropped to the console in sort form.");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified. Calling to this option makes --debug to be activated.");

	/* install exarg options */
	exarg_install_arg ("config", "f", EXARG_STRING, 
			   "Main server configuration location.");

	exarg_install_arg ("vortex-debug", NULL, EXARG_NONE,
			   "Enable vortex debug");

	exarg_install_arg ("vortex-debug2", NULL, EXARG_NONE,
			   "Enable second level vortex debug");

	exarg_install_arg ("vortex-debug-color", NULL, EXARG_NONE,
			   "Makes vortex debug to be done using colors according to the message type. If this variable is activated, vortex-debug variable is activated implicitly.");

	exarg_install_arg ("conf-location", NULL, EXARG_NONE,
			   "Allows to get the location of the turbulence configuration file that will be used by default");

	exarg_install_arg ("detach", NULL, EXARG_NONE,
			   "Makes turbulence to detach from console, starting in background.");

	exarg_install_arg ("child", NULL, EXARG_STRING,
			   "Internal flag used to create childs by the master process.");

	exarg_install_arg ("child-cmd-prefix", NULL, EXARG_STRING,
			   "Allows to configure an optional command prefix appended to the child starting command");

	exarg_install_arg ("disable-sigint", NULL, EXARG_NONE,
			   "Allows to disable SIGINT handling done by Turbulence. This option is only useful for debugging purposes..");

	exarg_install_arg ("no-unmap-modules", NULL, EXARG_NONE,
			   "Makes turbulence to no unmap module after exit (useful for debugging because symbols are available). This option is only useful for debugging purposes..");

	exarg_install_arg ("wait-thread-pool", NULL, EXARG_NONE,
			   "Makes turbulence to configure its vortex context to wait for threads from the pool to finish. By default turbulence will not wait. This option is only useful for debugging purposes..");


	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check for version request */
	if (exarg_is_defined ("version")) {
		printf ("%s\n", VERSION);
		/* terminates exarg */
		exarg_end ();
		return axl_false;
	}

	/* check for conf-location option */
	if (exarg_is_defined ("conf-location")) {
		printf ("VERSION:          %s\n", VERSION);
		printf ("VORTEX_VERSION:   %s\n", VORTEX_VERSION);
		printf ("AXL_VERSION:      %s\n", AXL_VERSION);
		printf ("SYSCONFDIR:       %s\n", turbulence_sysconfdir (NULL));
		printf ("TBC_DATADIR:      %s\n", turbulence_datadir (NULL));
		printf ("RUNTIME_DATADIR:  %s\n", turbulence_runtime_datadir (NULL));
		printf ("Default configuration file: %s/turbulence/turbulence.conf", SYSCONFDIR);

		/* terminates exarg */
		exarg_end ();

		return axl_false;
	}	

	/* exarg properly configured */
	return axl_true;
}

/**
 * @internal Implementation to detach turbulence from console.
 */
void turbulence_detach_process (void)
{
#if defined(AXL_OS_UNIX)
	pid_t   pid;
	/* fork */
	pid = fork ();
	switch (pid) {
	case -1:
		error ("unable to detach process, failed to executing child process");
		exit (-1);
	case 0:
		/* running inside the child process */
		msg ("running child created in detached mode");
		return;
	default:
		/* child process created (parent code) */
		break;
	} /* end switch */
#elif defined(AXL_OS_WIN32)
	char                * command;
	char                * command2;
	DWORD                 result;
	int                   pid;
	STARTUPINFO           si;
	PROCESS_INFORMATION   pi;
	TCHAR szPath[MAX_PATH];

	/* get current file name location */
	if( !GetModuleFileName( NULL, szPath, MAX_PATH )) {
		error ("Cannot install service (%d)", GetLastError ());
		return;
	}
	/* build file name with full path */
	command = axl_strdup_printf ("%s --ghost %s", szPath, exarg_get_string ("ghost"));

	/* clear memory from variables */
	ZeroMemory( &pi, sizeof(PROCESS_INFORMATION));

	/* configure std err and std out */
	ZeroMemory( &si, sizeof(STARTUPINFO) );
	si.cb = sizeof(STARTUPINFO); 

	/* check to extend command with log file if defined */
	if (exarg_is_defined ("log-file")) {
		command2 = command;
		command  = axl_strdup_printf ("%s --log-file %s", command2, exarg_get_string ("log-file"));
		axl_free (command2);
	} /* end if */

	msg ("starting child ghost process with: %s (error: %d)", command, GetLastError ());
	result = CreateProcess (
		NULL,    /* No module name (use command line) */
		(LPTSTR) command, /* Command line */
		NULL,    /* Process handle not inheritable */
		NULL,    /* Thread handle not inheritable */
		false,    /* Set handle inheritance to TRUE (important)  */
		0,       /* No creation flags */
		NULL,    /* Use parent's environment block */
		NULL,    /* Use parent's starting directory  */
		&si,     /* Pointer to STARTUPINFO structure */
		&pi);    /* Pointer to PROCESS_INFORMATION structure */
	
	if (result == 0) {
		/* drop an error message */
		error ("Failed to execute child ghost: result=%d, GetLastError () = %d",
		       result, GetLastError ());
	} else {
		msg ("Session detach operation completed (CreateProcess success) result is %d\n", result);
	}
#else
	/* drop an error message */
	error ("Unable to perform detach process operation, operation not supported at (%s) %s:%d.",
	       __AXL_PRETTY_FUNCTION__, __AXL_LINE__, __AXL_FILE__);
#endif

	/* terminate current process */
	msg ("finishing parent process (created child: %d, parent: %d)..", pid, getpid ());
	exit (0);
	return;
}

/**
 * @internal Places current process identifier into the file provided
 * by the user.
 */
void turbulence_place_pidfile (void)
{
	FILE * pid_file = NULL;
	int    pid      = getpid ();
	char   buffer[20];
	int    size;
	int    value;

	/* open pid file or create it to place the pid file */
	pid_file = fopen (PIDFILE, "w");
	if (pid_file == NULL) {
		abort_error ("Unable to open pid file at: %s", PIDFILE);
		return;
	} /* end if */
	
	/* stringfy pid */
	size = axl_stream_printf_buffer (buffer, 20, NULL, "%d", pid);
	msg ("signaling PID %d at %s", pid, PIDFILE);
	value = fwrite (buffer, 1, size, pid_file);
	if (value != size) {
	        abort_error ("Unable to update pid file at: %s  (fwrite call failed)", PIDFILE);
		return;
	}

	fclose (pid_file);
	return;
}


/**
 * @internal Removes pid file to avoid poiting to another process with
 * the same pid after stoping turbulence.
 */ 
void turbulence_remove_pidfile (void)
{
	/* remove pid file */
	turbulence_unlink (PIDFILE);

	return;
}

void main_signal_received (int _signal) {
	/* default handling */
	turbulence_signal_received (ctx, _signal);
}

void __turbulence_vortex_log_handler (const char       * file,
				      int                line,
				      VortexDebugLevel   log_level,
				      const char       * message,
				      va_list            args)
{
	if (log_level != VORTEX_LEVEL_CRITICAL)
		return;
	error ("(vortex) %s:%d: %s", file, line, message); 
	return;
}

int main (int argc, char ** argv)
{
	char          * config;
	VortexCtx     * vortex_ctx;
	int             temp_fds[2];

	/*** init exarg library ***/
	if (! main_init_exarg (argc, argv))
		return 0;

	/* create the turbulence and vortex context */
	ctx        = turbulence_ctx_new ();
	vortex_ctx = vortex_ctx_new ();

	/* check for child flag */
	if (exarg_is_defined ("child")) {
		/* reconfigure signals */
		turbulence_signal_install (ctx, 
					   /* disable sigint */
					   axl_false, 
					   /* signal sighup */
					   axl_false,
					   main_signal_received);		

	} else {
		/*** configure signal handling ***/
		turbulence_signal_install (ctx, 
					   /* signal sigint handling according to console options */
					   ! exarg_is_defined ("disable-sigint"), 
					   /* enable sighup handling */
					   axl_true,
					   /* signal received */
					   main_signal_received);
	} /* end if */

	/* check and enable console debug options */
	main_common_enable_debug_options (ctx, vortex_ctx);

	/* check loopback interface and vortex_support_pipe () is working */
	if (vortex_support_pipe (vortex_ctx, temp_fds)) {
		error ("vortex_support_pipe () is failing. Without this API turbulence cannot work properly. Please, check your LOOPBACK interface is up and running, with a 127.0.0.1 address and no other firewall/SELinux block");
		return -1;
	} /* end if */
	/* close temporal sockets */
	vortex_close_socket (temp_fds[0]);
	vortex_close_socket (temp_fds[1]);

	/* show some debug info */
	if (exarg_is_defined ("child")) {
		msg ("CHILD: starting child with control path: %s", exarg_get_string ("child"));

		/* recover child information: to see about the format
		   about this child init string, look at the function
		   turbulence_process_connection_status_string, inside
		   turbulence-process.c */
		if (! turbulence_child_build_from_init_string (ctx, exarg_get_string ("child"))) {
			error ("Failed to recover child information from init child string received");
			return -1;
		}
 	}

	/* check and get config location */
	config = main_common_get_config_location (ctx, vortex_ctx);

	/* check detach operation */
	if (exarg_is_defined ("detach")) {
		turbulence_detach_process ();
		/* caller do not follow */
	}

	if (! exarg_is_defined ("child")) {
		/* check here if the user has asked to place the pidfile */
		turbulence_place_pidfile ();
	}

	

	/* init libraries */
	if (! turbulence_init (ctx, vortex_ctx, config)) {
		/* free config */
		axl_free (config);

		/* free turbulence ctx */
		turbulence_ctx_free (ctx);
		return -1;
	} /* end if */

	/* check if vortex log is enabled and if is not, catch critical messages */
	if (! vortex_log_is_enabled (vortex_ctx)) {
		/* notify we are enabling this to inform child process
		 * to avoid enabling this on command line */
		turbulence_ctx_set_data (ctx, "debug-was-not-requested", INT_TO_PTR (axl_true));

		/* enable log */
		vortex_log_enable (vortex_ctx, axl_true); 
		vortex_log_set_prepare_log (vortex_ctx, axl_true);
		vortex_log_set_handler (vortex_ctx, __turbulence_vortex_log_handler);
	} /* end if */

	
	msg ("initial turbulence initialization ok, reading configuration files..");

	/* free config */
	axl_free (config);

	/* not required to free config var, already done by previous
	 * function */
	msg ("about to startup configuration found..");
	if (! turbulence_run_config (ctx)) {
		error ("Failed to run current config, finishing process: %d", getpid ());
		return -1;
	}

	/* post init child operations */
	if (exarg_is_defined ("child")) {
		if (! turbulence_child_post_init (ctx)) {
			error ("Failed to complete post init child operations, unable to start child process, killing: %d", getpid ());
			goto release_resources;
		}
	}

	/* drop a log */
	msg ("%sTurbulence STARTED OK (pid: %d, vortex ctx refs: %d)", exarg_is_defined ("child") ? "CHILD: " : "", getpid (),
	     vortex_ctx_ref_count (vortex_ctx));

	/* look main thread until finished */
	ctx->started = axl_true;
	vortex_listener_wait (vortex_ctx);

	msg ("   %sFinishing turbulence process, vortex_listener_wait unlocked (pid: %d, vortex ctx refs: %d)", 
	     exarg_is_defined ("child") ? "CHILD: " : "",
	     getpid (),
	     vortex_ctx_ref_count (vortex_ctx));
	
	/* terminate turbulence execution */
 release_resources:
	turbulence_exit (ctx, axl_false, axl_false);

	/* terminate exarg */
	msg ("terminating exarg library..");
	exarg_end ();

	/* free context (the very last operation) */
	vortex_ctx_free (vortex_ctx);

	if (! exarg_is_defined ("child")) {
		/* remote pid state file */
		turbulence_remove_pidfile ();
	} /* end if */

	turbulence_ctx_free (ctx);

	return 0;
}

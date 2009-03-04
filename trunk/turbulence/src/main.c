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

/* libturbulence support */
#include <turbulence.h>

/* command line argument parsing support */
#include <exarg.h>

/* system includes */
#include <signal.h>

#define HELP_HEADER "Turbulence: BEEP application server\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/* global instance created */
TurbulenceCtx * ctx;

/** 
 * @internal Init for all exarg functions provided by the turbulence
 * command line.
 */
int  main_init_exarg (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

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

	exarg_install_arg ("disable-sigint", NULL, EXARG_NONE,
			   "Allows to disable SIGINT handling done by Turbulence. This option is only useful for debugging purposes..");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check for conf-location option */
	if (exarg_is_defined ("conf-location")) {
		printf ("VERSION:         %s\n", VERSION);
		printf ("VORTEX_VERSION:  %s\n", VORTEX_VERSION);
		printf ("AXL_VERSION:     %s\n", AXL_VERSION);
		printf ("SYSCONFDIR:      %s\n", turbulence_sysconfdir ());
		printf ("TBC_DATADIR:     %s\n", turbulence_datadir ());
		printf ("Default configuration file: %s/turbulence/turbulence.conf", SYSCONFDIR);

		/* terminates exarg */
		exarg_end ();

		return axl_false;
	}	

	/* exarg properly configured */
	return axl_true;
}

/** 
 * @brief Termination signal received, notify.
 * @param value The signal received.
 */
void main_terminate_signal_received (int value)
{
	/* notify */
	turbulence_signal_exit (ctx, value);

	return;	
}

/**
 * @brief Reconf signal received, notify.
 * @param value The signal received.
 */
void main_reconf_signal_received (int value)
{
	/* notify */
	turbulence_reload_config (ctx, value);

#if defined(AXL_OS_UNIX)
	/* reinstall signal */
	signal (SIGHUP,  main_reconf_signal_received);
#endif
	
	return;
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

	/* open pid file or create it to place the pid file */
	pid_file = fopen (PIDFILE, "w");
	if (pid_file == NULL) {
		error ("Unable to open pid file at: %s", PIDFILE);
		return;
	} /* end if */
	
	/* stringfy pid */
	size = axl_stream_printf_buffer (buffer, 20, NULL, "%d", pid);
	msg ("signaling PID %d at %s", pid, PIDFILE);
	fwrite (buffer, size, 1, pid_file);

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

int main (int argc, char ** argv)
{
	char          * config;
	VortexCtx     * vortex_ctx;

	/*** init exarg library ***/
	if (! main_init_exarg (argc, argv))
		return 0;

	/* create the turbulence and vortex context */
	ctx        = turbulence_ctx_new ();
	vortex_ctx = vortex_ctx_new ();

	/*** configure signal handling ***/
	if (! exarg_is_defined ("disable-sigint"))
		signal (SIGINT,  main_terminate_signal_received); 
	signal (SIGSEGV, main_terminate_signal_received);
	signal (SIGABRT, main_terminate_signal_received);
	signal (SIGTERM, main_terminate_signal_received); 
#if defined(AXL_OS_UNIX)
	signal (SIGKILL, main_terminate_signal_received);
	signal (SIGQUIT, main_terminate_signal_received);
	signal (SIGHUP,  main_reconf_signal_received);
#endif


	/* configure context debug according to values received */
	turbulence_log_enable  (ctx, exarg_is_defined ("debug"));
	turbulence_log2_enable (ctx, exarg_is_defined ("debug2"));
	turbulence_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* enable vortex debug: do this at this place because
	 * turbulece_init makes a call to vortex_init */
	vortex_log_enable  (vortex_ctx, exarg_is_defined ("vortex-debug"));
	vortex_log2_enable (vortex_ctx, exarg_is_defined ("vortex-debug2"));
	if (exarg_is_defined ("vortex-debug-color")) {
		vortex_log_enable       (vortex_ctx, axl_true);
		vortex_color_log_enable (vortex_ctx, axl_true);
	} /* end if */

	/* check console color debug */
	turbulence_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* enable --debug option if it is found to be defined
	 * --color-debug */
	if (exarg_is_defined ("color-debug"))
		turbulence_log_enable (ctx, axl_true);

	/* init the vortex support module to allow finding the
	 * configuration file */
	vortex_support_init (vortex_ctx);

	/* configure lookup domain, and load configuration file */
	vortex_support_add_domain_search_path_ref (vortex_ctx, axl_strdup ("turbulence-conf"), 
						   vortex_support_build_filename (SYSCONFDIR, "turbulence", NULL));
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-conf", ".");

	/* find the configuration file */
	if (exarg_is_defined ("config")) {
		/* get the configuration defined at the command line */
		config = axl_strdup (exarg_get_string ("config"));
	} else {
		/* get the default configuration defined at
		 * compilation time */
		config = vortex_support_domain_find_data_file (vortex_ctx, "turbulence-conf", "turbulence.conf");
	} /* end if */

	/* load main turb */
	if (config == NULL) {
		abort_error ("Unable to find turbulence.conf file at the default location: %s/turbulence/turbulence.conf", SYSCONFDIR);
		return -1;
	} else 
		msg ("using configuration file: %s", config);

	/* check detach operation */
	if (exarg_is_defined ("detach")) {
		turbulence_detach_process ();
		/* caller do not follow */
	}

	/* check here if the user has asked to place the pidfile */
	turbulence_place_pidfile ();

	/* init libraries */
	if (! turbulence_init (ctx, vortex_ctx, config)) {
		/* free config */
		axl_free (config);

		/* free turbulence ctx */
		turbulence_ctx_free (ctx);
		return -1;
	} /* end if */
	
	msg ("initial turbulence initialization ok, reading configuration files..");

	/* free config */
	axl_free (config);

	/* not required to free config var, already done by previous
	 * function */
	msg ("about to startup configuration found..");
	if (! turbulence_run_config (ctx))
		return -1;

	/* drop a log */
	msg ("Turbulence STARTED OK");

	/* look main thread until finished */
	vortex_listener_wait (vortex_ctx);
	
	/* terminate turbulence execution */
	turbulence_exit (ctx, axl_false, axl_false);

	/* terminate exarg */
	msg ("terminating exarg library..");
	exarg_end ();

	/* free context (the very last operation) */
	turbulence_ctx_free (ctx);
	vortex_ctx_free (vortex_ctx);

	/* remote pid state file */
	turbulence_remove_pidfile ();

	return 0;
}

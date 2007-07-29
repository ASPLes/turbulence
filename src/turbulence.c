/*  Turbulence:  BEEP application server
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.turbulence.ws
 */
#if defined(ENABLE_TERMIOS)
# include <termios.h>
# include <sys/stat.h>
# include <unistd.h>
# define TBC_TERMINAL "/dev/tty"
#endif

#include <turbulence.h>
#include <signal.h>

#if defined(__COMPILING_TURBULENCE__) && defined(__GNUC__)
/* make happy gcc compiler */
int fsync (int fd);
#endif

/**
 * \defgroup turbulence Turbulence: main turbulence module, general facilities, initialization, etc.
 */

/**
 * \addtogroup turbulence.
 * @{
 */

#define HELP_HEADER "Turbulence: BEEP application server\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/** 
 * @internal Controls if messages must be send to the console log.
 */
bool        console_enabled        = false;
bool        console_debug          = false;
bool        console_debug2         = false;
bool        console_debug3         = false;
bool        console_color_debug    = false;
int         turbulence_pid         = -1;

bool        turbulence_is_existing = false;
VortexMutex turbulence_exit_mutex;

/** 
 * Starts turbulence execution, initializing all libraries required by
 * the server application.
 *
 * A call to turbulence_exit is required before exit.
 */
bool turbulence_init (int argc, char ** argv)
{
	/*** init exarg library ***/

	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("config", "f", EXARG_STRING, 
			   "Main server configuration location.");

	/* install console options */
	console_debug = true;
	turbulence_console_install_options (); 

	exarg_install_arg ("vortex-debug", NULL, EXARG_NONE,
			   "Enable vortex debug");

	exarg_install_arg ("vortex-debug-color", NULL, EXARG_NONE,
			   "Makes vortex debug to be done using colors according to the message type. If this variable is activated, vortex-debug variable is activated implicitly.");

	exarg_install_arg ("conf-location", NULL, EXARG_NONE,
			   "Allows to get the location of the turbulence configuration file that will be used by default");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check if the console is defined */
	turbulence_console_process_options (); 

	/* enable vortex debug */
	vortex_log_enable (exarg_is_defined ("vortex-debug"));
	if (exarg_is_defined ("vortex-debug-color")) {
		vortex_log_enable       (true);
		vortex_color_log_enable (exarg_is_defined ("vortex-debug-color"));
	} /* end if */

	/*** init the vortex library ***/
	if (! vortex_init ()) {
		error ("unable to start vortex library, terminating turbulence execution..");
		return false;
	} /* end if */

	/*** not required to initialize axl library, already done by vortex ***/

	msg ("turbulence internal init");

	/* configure lookup domain for turbulence data */
	vortex_support_add_domain_search_path_ref (axl_strdup ("turbulence-data"), 
						   vortex_support_build_filename (DATADIR, "turbulence", NULL));
	vortex_support_add_domain_search_path     ("turbulence-data", ".");

	vortex_mutex_create (&turbulence_exit_mutex);
	turbulence_module_init ();
	turbulence_db_list_init ();

	/* install default signal handling */
	signal (SIGINT,  turbulence_exit);
	signal (SIGTERM, turbulence_exit);
	signal (SIGKILL, turbulence_exit);
	signal (SIGQUIT, turbulence_exit);
	signal (SIGSEGV, turbulence_exit);
	signal (SIGABRT, turbulence_exit);
	signal (SIGHUP,  turbulence_reload_config);

	/* init ok */
	return true;
} /* end if */

/** 
 * @internal Function that performs a reload operation for the current
 * turbulence instance.
 * 
 * @param value The signal number caught.
 */
void turbulence_reload_config (int value)
{
	bool already_notified = false;
	msg ("caught HUP signal, reloading configuration");
	/* reconfigure signal received, notify turbulence modules the
	 * signal */
	vortex_mutex_lock (&turbulence_exit_mutex);
	if (already_notified) {
		vortex_mutex_unlock (&turbulence_exit_mutex);
		return;
	}
	already_notified = true;

	/* reload turbulence here, before modules
	 * reloading */
	turbulence_db_list_reload_module ();
	
	/* reload modules */
	turbulence_module_notify_reload_conf ();
	vortex_mutex_unlock (&turbulence_exit_mutex);

	/* reload signal */
	signal (SIGHUP,  turbulence_reload_config);
	
	return;
} 

/** 
 * Terminates the turbulence excution, returing the exit value
 * provided as first parameter.
 * 
 * @param value The exit code to return.
 */
void turbulence_exit (int value)
{
	axlDoc           * doc;
	axlNode          * node;
	VortexAsyncQueue * queue;

	/* lock the mutex and check */
	vortex_mutex_lock (&turbulence_exit_mutex);
	if (turbulence_is_existing) {
		/* other thread is already cleaning */
		vortex_mutex_unlock (&turbulence_exit_mutex);
		return;
	} /* end if */

	/* flag that turbulence is existing and do all cleanup
	 * operations */
	turbulence_is_existing = true;
	vortex_mutex_unlock (&turbulence_exit_mutex);
	
	switch (value) {
	case SIGINT:
		msg ("caught SIGINT, terminating turbulence..");
		break;
	case SIGTERM:
		msg ("caught SIGTERM, terminating turbulence..");
		break;
	case SIGKILL:
		msg ("caught SIGKILL, terminating turbulence..");
		break;
	case SIGQUIT:
		msg ("caught SIGQUIT, terminating turbulence..");
		break;
	case SIGSEGV:
	case SIGABRT:
		error ("caught %s, anomalous termination (this is an internal turbulence or module error)",
		       value == SIGSEGV ? "SIGSEGV" : "SIGABRT");
		
		/* check current termination option */
		doc  = turbulence_config_get ();
		node = axl_doc_get (doc, "/turbulence/global-settings/on-bad-signal");
		if (HAS_ATTR_VALUE (node, "action", "ignore")) {
			/* ignore the signal emision */
			return;
		} else if (HAS_ATTR_VALUE (node, "action", "hold")) {
			/* lock the process */
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
	vortex_listener_unlock ();

	return;
}

/** 
 * @internal Performs all operations required to cleanup turbulence
 * runtime execution.
 */
void turbulence_cleanup ()
{
	msg ("cleaning up..");

	/* terminate all modules */
	turbulence_module_cleanup ();
	turbulence_config_cleanup ();
	turbulence_db_list_cleanup ();

	/* terminate vortex */
	msg ("terminating vortex library..");
	vortex_exit ();

	/* terminate profile path */
	turbulence_ppath_cleanup ();
	
	/* terminate exarg */
	msg ("terminating exarg library..");
	exarg_end ();

	/* the last module to clean up */
	turbulence_log_cleanup ();

	/* terminate */
	vortex_mutex_destroy (&turbulence_exit_mutex);

	return;
}

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLE if (console_enabled) fprintf

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLEV if (console_enabled) vfprintf


/** 
 * @brief Process all console debug options activated the by the user.
 */
void turbulence_console_process_options ()
{
	/* get current process id */
	turbulence_pid = getpid ();

	/* activate log console */
	console_debug       = exarg_is_defined ("debug");
	console_debug2      = exarg_is_defined ("debug2");
	console_debug3      = exarg_is_defined ("debug3");
	console_enabled     = console_debug || console_debug2 || console_debug3;

	/* implicitly activate third and second console debug */
	if (console_debug3)
		console_debug2 = true;
	if (console_debug2)
		console_debug = true;
	
	console_color_debug = exarg_is_defined ("color-debug");

	return;
}

/** 
 * @brief Install default console debug options accepted for
 * turbulence tools. Activates by default the --debug option which is
 * the normal console output.
 */
void turbulence_console_install_options ()
{
	/* install default debug options. */

	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Makes all log produced by the application, to be also dropped to the console in sort form.");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified.");

	/* check implicit activation */
	if (console_debug) 
		console_debug = false;
	else {
		/* seems the caller is a tool, define the debug flag
		 * to be activated. */
		exarg_define ("debug", NULL);
	}
	return;
}

/** 
 * @internal function that actually handles the console error.
 */
void turbulence_error (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	
	/* check extended console log */
	if (console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (exarg_is_defined ("color-debug")) {
			CONSOLE (stderr, "(proc:%d) [\e[1;31merr\e[0m] (%s:%d) ", turbulence_pid, file, line);
		} else
#endif
			CONSOLE (stderr, "(proc:%d) [err] (%s:%d) ", turbulence_pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stderr, "\e[1;31mE: \e[0m");
		} else
#endif
			CONSOLE (stderr, "E: ");
	} /* end if */
	
	va_start (args, format);

	/* report to the console */
	CONSOLEV (stderr, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);
	
	va_end (args);

	CONSOLE (stderr, "\n");
	
	fflush (stderr);
	
	return;
}

/** 
 * @internal function that actually handles the console msg.
 */
void turbulence_msg (const char * file, int line, const char * format, ...)
{
	va_list args;

	/* check extended console log */
	if (console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", turbulence_pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", turbulence_pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console msg (second level debug)
 */
void turbulence_msg2 (const char * file, int line, const char * format, ...)
{
	va_list args;

	/* check second level debug */
	if (! console_debug2)
		return;

	/* check extended console log */
	if (console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (exarg_is_defined ("color-debug")) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", turbulence_pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", turbulence_pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn.
 */
void turbulence_wrn (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	/* check extended console log */
	if (console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (exarg_is_defined ("color-debug")) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", turbulence_pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", turbulence_pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn_sl.
 */
void turbulence_wrn_sl (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	/* check extended console log */
	if (console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (exarg_is_defined ("color-debug")) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", turbulence_pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", turbulence_pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	fflush (stdout);
	
	return;
}

/** 
 * @brief Provides the same functionality like \ref turbulence_file_test_v,
 * but allowing to provide the file path as a printf like argument.
 * 
 * @param format The path to be checked.
 * @param test The test to be performed. 
 * 
 * @return true if all test returns true. Otherwise false is returned.
 */
bool turbulence_file_test_v (const char * format, VortexFileTest test, ...)
{
	va_list   args;
	char    * path;
	bool      result;

	/* open arguments */
	va_start (args, test);

	/* get the path */
	path = axl_strdup_printfv (format, args);

	/* close args */
	va_end (args);

	/* do the test */
	result = vortex_support_file_test (path, test);
	axl_free (path);

	/* return the test */
	return result;
}


/** 
 * @brief Creates the directory with the path provided.
 * 
 * @param path The directory to create.
 * 
 * @return true if the directory was created, otherwise false is
 * returned.
 */
bool     turbulence_create_dir  (const char * path)
{
	/* check the reference */
	if (path == NULL)
		return false;
	
	/* create the directory */
	return (mkdir (path, 770) == 0);
}

/** 
 * @brief Allows to get the modification for the provided format,
 * which can be used to detect file changes, to perform the required
 * operations.
 * 
 * @param file The file to check for its modification time.
 * 
 * @return The last modification time for the file provided. The
 * function return -1 if it fails.
 */
long int turbulence_last_modification (const char * file)
{
	struct stat status;

	/* check file received */
	if (file == NULL)
		return -1;

	/* get stat */
	if (stat (file, &status) == 0)
		return (long int) status.st_mtime;
	
	/* failed to get stats */
	return -1;
}


/*
 * @brief Allows to get the next line read from the user. The function
 * return an string allocated.
 * 
 * @return An string allocated or NULL if nothing was received.
 */
char * turbulence_io_get (char * prompt, TurbulenceIoFlags flags)
{
	struct termios current_set;
	struct termios new_set;
	int input;
	int output;

	/* buffer declaration */
	char   buffer[1024];
	char * result = NULL;
	int    iterator;
	
	/* try to read directly from the tty */
	if ((input = output = open (TBC_TERMINAL, O_RDWR)) < 0) {
		/* if fails to open the terminal, use the standard
		 * input and standard error */
		input  = STDIN_FILENO;
		output = STDERR_FILENO;
	} /* end if */

	/* print the prompt if defined */
	if (prompt != NULL) {
		/* write the prompt */
		write (output, prompt, strlen (prompt));
		fsync (output);
	}

	/* check to disable echo */
	if (flags & DISABLE_STDIN_ECHO) {
		if (input != STDIN_FILENO && (tcgetattr (input, &current_set) == 0)) {
			/* copy to the new set */
			memcpy (&new_set, &current_set, sizeof (struct termios));
			
			/* configure new settings */
			new_set.c_lflag &= ~(ECHO | ECHONL);
			
			/* set this values to the current input */
			tcsetattr (input, TCSANOW, &new_set);
		} /* end if */
	} /* end if */

	iterator = 0;
	memset (buffer, 0, 1024);
	/* get the next character */
	while ((iterator < 1024) && (read (input, buffer + iterator, 1) == 1)) {
		
		if (buffer[iterator] == '\n') {
			/* remove trailing \n */
			buffer[iterator] = 0;
			result = axl_strdup (buffer);

			break;
		} /* end if */


		/* update the iterator */
		iterator++;
	}

	/* return terminal settings if modified */
	if (flags & DISABLE_STDIN_ECHO) {
		if (input != STDIN_FILENO) {
			
			/* set this values to the current input */
			tcsetattr (input, TCSANOW, &current_set);

			/* close opened file descriptor */
			close (input);
		} /* end if */
	} /* end if */

	/* do not return anything from this point */
	return result;
}

/* @} */

/**
 * \mainpage 
 *
 * To be filled.
 */

/**
 * \page turbulence_mod_sasl Turbunece mod-sasl: SASL profile support for turbulence
 *
 * FIXME
 */

/**
 * \page turbulence_mod_tunnel Turbulence mod-tunnel: TUNNEL profile support for turbulence
 *
 * FIXME
 */

/**
 * \page turbulence_tools Turbulence tools: Set of tools included
 * inside turbulence to manage and extend its function.
 *
 * FIXME
 */



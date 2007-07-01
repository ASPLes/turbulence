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
 *         info@aspl.es - http://fact.aspl.es
 */
#if defined(ENABLE_TERMIOS)
# include <termios.h>
# include <sys/stat.h>
# include <unistd.h>
# define TBC_TERMINAL "/dev/tty"
#endif

#include <turbulence.h>
#include <signal.h>

#define HELP_HEADER "Turbulence: BEEP application server\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/** 
 * @internal Controls if messages must be send to the console log.
 */
bool        console_enabled = false;
int         turbulence_pid  = -1;

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

	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Makes all log produced by the application, to be also dropped to the console");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified");

	exarg_install_arg ("vortex-debug", NULL, EXARG_NONE,
			   "Enable vortex debug");

	exarg_install_arg ("vortex-debug-color", NULL, EXARG_NONE,
			   "Makes vortex debug to be done using colors according to the message type. If this variable is activated, vortex-debug variable is activated implicitly.");

	exarg_install_arg ("conf-location", NULL, EXARG_NONE,
			   "Allows to get the location of the turbulence configuration file that will be used by default");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* check if the console is defined */
	turbulence_set_console_debug (exarg_is_defined ("debug"));

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
	vortex_mutex_create (&turbulence_exit_mutex);
	turbulence_module_init ();

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
 * @brief Allows to activated/deactivate the console log reporting.
 * 
 * @param debug The value to be configured, false to disable console
 * log, true to enable it.
 */
void turbulence_set_console_debug (bool debug)
{
	/* get current process id */
	turbulence_pid = getpid ();

	/* set the value */
	console_enabled = debug;
}

/** 
 * @internal function that actually handles the console error.
 */
void turbulence_error (const char * file, int line, const char * format, ...)
{
	va_list args;
	
	
#if defined(AXL_OS_UNIX)	
	if (exarg_is_defined ("color-debug")) {
		CONSOLE (stderr, "(proc:%d) [\e[1;31merr\e[0m] (%s:%d) ", turbulence_pid, file, line);
	} else
#endif
	CONSOLE (stderr, "(proc:%d) [err] (%s:%d) ", turbulence_pid, file, line);
	
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

#if defined(AXL_OS_UNIX)	
	if (exarg_is_defined ("color-debug")) {
		CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", turbulence_pid, file, line);
	} else
#endif
		CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", turbulence_pid, file, line);
	
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
	
#if defined(AXL_OS_UNIX)	
	if (exarg_is_defined ("color-debug")) {
		CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", turbulence_pid, file, line);
	} else
#endif
	CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", turbulence_pid, file, line);
	
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
	
#if defined(AXL_OS_UNIX)	
	if (exarg_is_defined ("color-debug")) {
		CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", turbulence_pid, file, line);
	} else
#endif
	CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", turbulence_pid, file, line);
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	/* report to log */
	turbulence_log_report (LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	fflush (stdout);
	
	return;
}

/** 
 * @brief Provides the same functionality like \ref turbulence_file_test,
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

/**
 * \mainpage 
 *
 * \section intro Introduction
 *
 * Turbulence is a general server side application that implements
 * many of the administrative tasks required while deploying BEEP
 * applications based on Vortex Library. Because Vortex Library
 * (http://vortex.aspl.es) only provides support for the BEEP
 * protocol, a server side application is required on top of it to
 * init the library and to provide the required provisioning,
 * connecting the protocol to the real problem to be solved.
 * 
 * Turbulence is extended through modules, and already provides into
 * its base code modules to manage SASL users and an implementation of
 * the TUNNEL profile.
 * 
 * The following are a set of documents that provides information
 * about using Turbulence and to extend it.
 * 
 * - \ref turbulence_configuration
 *
 * 
 */

/**
 * \page turbulence_configuration Configuring Turbulence
 *
 * \section Index
 *
 *   - \ref turbulence_configuration_file
 *   - \ref turbulence_addresses_and_ports
 *   - \ref turbulence_log_files
 *   - \ref turbulence_modules_configuration
 *   - \ref turbulence_profile_path
 *
 * \section turbulence_configuration_file How turbulence is configured (configuration file location)
 * 
 * Turbulence is configured through XML 1.0 files. According to your
 * installation, the main turbulence configuration file should be
 * found at the location provided by the following command:
 * 
 * \code
 * turbulence --conf-location --debug
 * (proc:13369) [msg] (turbulence.c:120) turbulence internal init
 * (proc:13369) [msg] (main.c:50) Default configuration file: /etc/turbulence/turbulence.conf
 * \endcode
 * 
 * Alternatively you can provide your own configuration file by using
 * the <b>--config</b> option: 
 * 
 * \code
 * turbulence --conf my-turbulence.conf
 * \endcode
 *
 * This main configuration file includes several main sections:
 * 
 * - <b>&lt;global-settings></b>: This main section includes several
 * run time configuration for the BEEP server: TCP ports, listener
 * address, log files, crash handling, etc.
 *
 * - <b>&lt;modules></b>: following to this section is found the
 * modules configuration. Under this section is mainly configured
 * directories holding modules.
 *
 * - <b>&lt;profile-path-configuration></b>: and finally, at the end
 * of the file is found the profile path configuration. This is one of
 * the great features provided by turbulence, an administrative
 * configuration which allows to mix, sequence, and select profiles to
 * be provided to client peers according to several run time values
 * such remote address, profiles already initiated.
 * 
 * \section turbulence_addresses_and_ports Turbulence addresses and ports
 *
 * Ports and addresses used by Turbulence to listen are configured at
 * the <b>&lt;global-settings></b> section. Here is an example:
 * 
 * <div class="xml-doc">
 * \code
 *  <global-settings>
 *    <!-- ... more settings ... -->
 * 
 *    <!-- port to listen to -->
 *    <ports>
 *      <port>3206</port>
 *      <port>44010</port>
 *    </ports>
 *
 *    <!-- listener configuration (address to listen) --> 
 *    <listener>
 *      <name>0.0.0.0</name>
 *    </listener>
 *  </global-settings>
 * \endcode
 * </div>
 *
 * Previous example will make Turbulence to listen on ports 3206 and
 * 44010 for all addresses that are known for the server hosting
 * turbulence (0.0.0.0). Turbulence will understand this section
 * listening on all addresses provided, for all ports.
 *
 * Then you can use profile path to enforce which profiles will be
 * served on each port. For example, you can provide TUNNEL support
 * only on port 3206.
 * 
 * \section turbulence_log_files Configuring turbulence log files
 * 
 * Log files used by turbulence are configured at the
 * <b>&lt;global-settings</b> section. FIXME ME, FINISH TURBULENCE LOG
 * SUPPORT.
 *
 * \section turbulence_modules_configuration Turbulence modules configuration
 * 
 * Modules loaded by turbulence are found at the directories
 * configured at the <b>&lt;modules></b> section. Here is an example:
 * 
 * <div class="xml-doc">
 * \code
 *  <modules>
 *    <!-- directory where to find modules to load -->
 *    <directory src="/etc/turbulence/mods-enabled" /> 
 *  </modules>
 * \endcode
 * </div>
 * 
 * Every directory configured contains turbulence xml module pointers
 * having the following content:
 * 
 * <div class="xml-doc">
 * \code
 * <mod-turbulence location="/usr/lib/turbulence/modules/mod-sasl.so"/>
 * \endcode
 * </div>
 * 
 * Each module have its own configuration file, which should use XML
 * as default configuration format. Check the following documents for
 * modules implemented into Turbulence:
 * 
 * - \ref turbulence_mod_sasl
 * - \ref turbulence_mod_tunnel
 *  
 *
 * 
 * 
 * \section turbulence_profile_path Profile path configuration
 *
 * Profile Path is a feature that will allow to configure which
 * profiles can be used by remote peers, according to several run time
 * configurations. It is designed to make it more easy to develop a
 * BEEP profile, to them mix it with other profiles in many ways
 * making them more useful through the combination created at
 * run-time.
 * 
 * Let's see some examples to initially clarify the profile path
 * support. A usual configuration around BEEP is to provide SASL
 * autentication and then allow using a profile which do useful
 * work. With profile path this can be configured as:
 * 
 * <div class="xml-doc">
 * \code
 * <path-def server-name=".*" src="not 192.168.0.*" path-name="not local-parts">
 *   <if-success profile="http://iana.org/beep/SASL/*" connmark="sasl:is:authenticated">
 *      <allow profile="http://iana.org/beep/TUNNEL" />
 *   </if-success>
 * </path-def>
 * \endcode
 * </div>
 *
 * Previous example instruct Turbulence to apply a profile path called
 * "not local-parts" if the source of the connection comes not from
 * 192.168.0.X. It also teaches Turbulence to only provide SASL
 * profiles and once initiated properly (and autenticated), the remote
 * client peer can use the TUNNEL profile. Nothing more.
 *
 * Profile path is applied as a chain, composed by <b>&lt;path-def></b>
 * definitions. Once a path-def match the target, the profile path
 * configuration found on it its applied to the peer. 
 * 
 * \image html profile-path-chain.png "Profile path chain"
 *
 * As the previous image shows, the turbulence profile path
 * configuration is composed by several profile path definitions:
 * 
 * <div class="xml-doc">
 * \code
 * <path-def server-name=".*" src="not 192.168.0.*" path-name="not local-parts">
 *   <!-- profile path configuration -->
 * </path-def>
 * \endcode
 * </div>
 * 
 * It supports the following matching configurations:
 * 
 * - <b>server-name</b>: a perl expression defining the the serverName
 * to match for the connection.
 *
 * - <b>src</b>: a perl expression defining the source of the
 * connection.
 *
 * - <b>path-name</b>: an administrative flag that will help to
 * recognize the connection in future process (log reporting).
 *
 * Once a path is matched, the following are discarded and the
 * configuration inside the profile path is applied to the connection
 * as long as the connection is running.
 * 
 * Now, it is required to configure which profiles will be
 * allowed. This is done by using two types of nodes:
 * 
 * - <b>&lt;if-success></b>: a configuration that allows to use the
 * profile referenced by it, and additionally, allows to use profiles
 * provided by other nodes inside it once a channel with the profile
 * provided is initiated.
 *
 * - <b>&lt;allow></b>: a configuration that allows to use a profile
 * at a particular level.
 *
 * Let's see an example to show how it works:
 * 
 * <div class="xml-doc">
 * \code
 * <if-success profile="http://iana.org/beep/SASL/.*" connmark="sasl:is:authenticated" >
 *    <allow profile="http://turbulence.ws/profiles/test1" preconnmark="sasl:is:authenticated"/>
 *    <if-success profile="http://iana.org/beep/TLS" >
 *       <allow profile="http://iana.org/beep/xmlrpc" />
 *       <allow profile="http://fact.aspl.es/profiles/coyote_profile" />
 *    </if-success>
 * </if-success>
 * \endcode
 * </div>
 *
 * Previous example have configured a particular profile path as
 * follows:
 *
 * \image html profile-path-example.png "Profile path example: how it is applied profile path"
 * 
 * The example is mostly self-explanatory, but one detail remains. The
 * configuration uses two attributes: <b>connmark</b> and
 * <b>preconnmark</b>. They are used as flags that must be detected in
 * the connection in other to allow the connection to accept a profile
 * or the content of the following profiles.
 *
 * - <b>preconnmark</b>: if used, it means that the connection must
 * have the "mark" provided before the profile can be accepted to be
 * initiated.
 * 
 * - <b>connmark</b>: can only be used from &lt;if-success> nodes, and
 * allows to protect the profile path content inside the
 * &lt;if-success> node, enforcing to not only create a channel under
 * the profile provided but also having the mark. This is particular
 * useful for the SASL case because having a SASL channel created
 * isn't a warranty of a connection authenticated. Thus, an additional
 * mark is required to properly ensure that the connection was
 * authenticated.
 *
 * So, a good question at this point is "how are those marks
 * created?". These marks are profile dependant and are created using
 * the interface provided by the vortex connection module to store
 * data:
 * 
 * - vortex_connection_set_data
 *
 * In this case, SASL implementation flags the connection as
 * authenticated using the provided flag: "sasl:is:authenticated".
 * 
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

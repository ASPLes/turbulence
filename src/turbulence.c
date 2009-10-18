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
#if defined(ENABLE_TERMIOS)
# include <termios.h>
# include <sys/stat.h>
# include <unistd.h>
# define TBC_TERMINAL "/dev/tty"
#endif

#include <turbulence.h>


#if defined(AXL_OS_UNIX)
/* used by turbulence_get_system_id */
# include <pwd.h>
# include <grp.h>

/* used by fchmod */
# include <sys/types.h>
# include <sys/stat.h>

#endif
#include <unistd.h>

/* local include */
#include <turbulence-ctx-private.h>

#if defined(__COMPILING_TURBULENCE__) && defined(__GNUC__)
/* make happy gcc compiler */
int fsync (int fd);
#endif

/** 
 * \defgroup turbulence Turbulence: general facilities, initialization, etc
 */

/** 
 * \addtogroup turbulence
 * @{
 */

/** 
 * @brief Starts turbulence execution, initializing all libraries
 * required by the server application.
 *
 * A call to \ref turbulence_exit is required before exit.
 */
int  turbulence_init (TurbulenceCtx * ctx, 
		      VortexCtx     * vortex_ctx,
		      const char    * config)
{
	/* no initialization done if null reference received */
	if (ctx == NULL) {
	        abort_error ("Received a null turbulence context, failed to init the turbulence");
		return axl_false;
	} /* end if */
	if (config == NULL) {
		abort_error ("Received a null reference to the turbulence configuration, failed to init the turbulence");
		return axl_false;
	} /* end if */

	/* get current process id */
	ctx->pid = getpid ();

	/* init turbulence internals */
	vortex_mutex_create (&ctx->exit_mutex);

	/* if a null value is received for the vortex context, create
	 * a new empty one */
	if (vortex_ctx == NULL) {
		msg2 ("creating a new vortex context because a null value was received..");
		vortex_ctx = vortex_ctx_new ();
	} /* end if */

	/* configure the vortex context created */
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);

	/* configure lookup domain for turbulence data */
	vortex_support_add_domain_search_path_ref (vortex_ctx, axl_strdup ("turbulence-data"), 
						   vortex_support_build_filename (TBC_DATADIR, "turbulence", NULL));
#if defined(AXL_OS_WIN32)
	/* make turbulence to add the path ../data to the search list
	 * under windows as it is organized this way */
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-data", TBC_DATADIR);
#endif
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-data", ".");

	/*** init the vortex library ***/
	if (! vortex_init_ctx (vortex_ctx)) {
		abort_error ("unable to start vortex library, terminating turbulence execution..");
		return axl_false;
	} /* end if */

	/* init turbulence-mediator.c: init this module before others
	   to acchieve notifications and push events  */
	turbulence_mediator_init (ctx);

	/*** not required to initialize axl library, already done by vortex ***/
	msg ("turbulence internal init ctx: %p, vortex ctx: %p", ctx, vortex_ctx);

	/* db list */
	if (! turbulence_db_list_init (ctx)) {
		abort_error ("failed to init the turbulence db-list module");
		return axl_false;
	} /* end if */

	/* init turbulence-module.c */
	turbulence_module_init (ctx);

	/* init turbulence-proces.c: reinit=axl_false */
	turbulence_process_init (ctx, axl_false);

	/* load current turbulence configuration */
	if (! turbulence_config_load (ctx, config)) {
		/* unable to load configuration */
		return axl_false;
	}

	/* init profile path module: this initialization must be done
	 * before calling to turbulence_run_config to avoid third
	 * party modules to install handler with higher priority. */
	if (! turbulence_ppath_init (ctx)) {
		return axl_false;
	} /* end if */

	/* init connection manager: reinit=axl_false */
	turbulence_conn_mgr_init (ctx, axl_false);

	/* init ok */
	return axl_true;
} /* end if */

/** 
 * @brief Function that performs a reload operation for the current
 * turbulence instance (represented by the provided TurbulenceCtx).
 * 
 * @param ctx The turbulence context representing a running instance
 * that must reload.
 *
 * @param value The signal number caught. This value is optional since
 * reloading can be triggered not only by a signal received (i.e.
 * SIGHUP).
 */
void     turbulence_reload_config       (TurbulenceCtx * ctx, int value)
{
	/* get turbulence context */
	int             already_notified = axl_false;
	
	msg ("caught HUP signal, reloading configuration");
	/* reconfigure signal received, notify turbulence modules the
	 * signal */
	vortex_mutex_lock (&ctx->exit_mutex);
	if (already_notified) {
		vortex_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	already_notified = axl_true;

	/* reload turbulence here, before modules
	 * reloading */
	turbulence_db_list_reload_module ();
	
	/* reload modules */
	turbulence_module_notify_reload_conf (ctx);
	vortex_mutex_unlock (&ctx->exit_mutex);

	return;
} 

/** 
 * @brief Performs all operations required to cleanup turbulence
 * runtime execution (calling to all module cleanups).
 */
void turbulence_exit (TurbulenceCtx * ctx, 
		      int             free_ctx,
		      int             free_vortex_ctx)
{
	VortexCtx * vortex_ctx;

	/* do not perform any change if a null context is received */
	v_return_if_fail (ctx);

	msg ("cleaning up..");

	/* check to kill childs */
	turbulence_process_kill_childs (ctx);

	/* terminate all modules */
	turbulence_config_cleanup (ctx);

	/* unref all connections (before calling to terminate vortex) */
	turbulence_conn_mgr_cleanup (ctx);

	/* get the vortex context assocaited */
	vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	/* terminate profile path */
	turbulence_ppath_cleanup (ctx);

	/* close modules before terminating vortex so they still can
	 * use the Vortex API to terminate its function. */
	turbulence_module_notify_close (ctx);

	/* terminate turbulence db list module at this point to avoid
	 * modules referring to db-list to lost references */
	turbulence_db_list_cleanup (ctx);

	/* terminate turbulence mediator module */
	turbulence_mediator_cleanup (ctx);

	/* do not release the context (this is done by the caller) */
	turbulence_log_cleanup (ctx);

	/* terminate vortex */
	msg ("now, terminate vortex library after turbulence cleanup..");
	vortex_exit_ctx (vortex_ctx, free_vortex_ctx);

	/* now modules and vortex library is stopped, terminate
	 * modules unloading them */
	turbulence_module_cleanup (ctx);

	/* terminate run module */
	turbulence_run_cleanup (ctx);

	/* cleanup process module */
	turbulence_process_cleanup (ctx);

	/* free mutex */
	vortex_mutex_destroy (&ctx->exit_mutex);

	/* free ctx */
	if (free_ctx)
		turbulence_ctx_free (ctx);

	return;
}

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLE if (ctx->console_enabled || ignore_debug) fprintf

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLEV if (ctx->console_enabled || ignore_debug) vfprintf



/** 
 * @internal function that actually handles the console error.
 *
 * @param ignore_debug Allows to configure if the debug configuration
 * must be ignored (bypassed) and drop the log. This can be used to
 * perform logging for important messages.
 */
void turbulence_error (TurbulenceCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	
	/* check extended console log */
	if (ctx->console_debug3 || ignore_debug) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "(proc:%d) [\e[1;31merr\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stderr, "(proc:%d) [err] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "\e[1;31mE: \e[0m");
		} else
#endif
			CONSOLE (stderr, "E: ");
	} /* end if */
	
	va_start (args, format);

	/* report to the console */
	CONSOLEV (stderr, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_ERROR, format, args, file, line);

	va_end (args);
	va_start (args, format);

	turbulence_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);
	
	va_end (args);

	CONSOLE (stderr, "\n");
	
	fflush (stderr);
	
	return;
}

/** 
 * @brief Allows to check if the debug is activated (\ref msg type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
int  turbulence_log_enabled (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug;
}

/** 
 * @brief Allows to activate the turbulence console log (by default
 * disabled).
 * 
 * @param ctx The turbulence context to configure.  @param value The
 * value to configure to enable/disable console log.
 */
void turbulence_log_enable       (TurbulenceCtx * ctx, 
				  int  value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_debug = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	return;
}




/** 
 * @brief Allows to check if the second level debug is activated (\ref
 * msg2 type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
int  turbulence_log2_enabled (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	v_return_val_if_fail (ctx, axl_false);
	
	return ctx->console_debug2;
}

/** 
 * @brief Allows to activate the second level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The turbulence context to configure.
 * @param value The value to configure.
 */
void turbulence_log2_enable      (TurbulenceCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug2 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug2)
		ctx->console_debug = axl_true;

	return;
}

/** 
 * @brief Allows to check if the third level debug is activated (\ref
 * msg2 with additional information).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
int  turbulence_log3_enabled (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug3;
}

/** 
 * @brief Allows to activate the third level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The turbulence context to configure.
 * @param value The value to configure.
 */
void turbulence_log3_enable      (TurbulenceCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug3 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug3)
		ctx->console_debug2 = axl_true;

	return;
}

/** 
 * @brief Allows to configure if the console log produced is colorfied
 * according to the status reported (red: (error,criticals), yellow:
 * (warning), green: (info, debug).
 * 
 * @param ctx The turbulence context to configure.
 *
 * @param value The value to configure. This function could take no
 * effect on system where ansi values are not available.
 */
void turbulence_color_log_enable (TurbulenceCtx * ctx,
				  int             value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_color_debug = value;

	return;
}

/** 
 * @internal function that actually handles the console msg.
 */
void turbulence_msg (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console access
 */
void  turbulence_access   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_ACCESS, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console msg (second level debug)
 */
void turbulence_msg2 (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* check second level debug */
	if (! ctx->console_debug2)
		return;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn.
 */
void turbulence_wrn (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);
	va_start (args, format);

	turbulence_log_report (ctx, LOG_REPORT_ERROR, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn_sl.
 */
void turbulence_wrn_sl (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get turbulence context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	turbulence_log_report (ctx, LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	fflush (stdout);
	
	return;
}

/** 
 * @brief Provides the same functionality like vortex_support_file_test,
 * but allowing to provide the file path as a printf like argument.
 * 
 * @param format The path to be checked.
 * @param test The test to be performed. 
 * 
 * @return axl_true if all test returns axl_true. Otherwise axl_false is returned.
 */
int  turbulence_file_test_v (const char * format, VortexFileTest test, ...)
{
	va_list   args;
	char    * path;
	int       result;

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
 * @return axl_true if the directory was created, otherwise axl_false is
 * returned.
 */
int      turbulence_create_dir  (const char * path)
{
	/* check the reference */
	if (path == NULL)
		return axl_false;
	
	/* create the directory */
#if defined(AXL_OS_WIN32)
	return (_mkdir (path) == 0);
#else
	return (mkdir (path, 0770) == 0);
#endif
}

/**
 * @brief Allows to remove the selected file pointed by the path
 * provided.
 *
 * @param path The path to the file to be removed.
 *
 * @return axl_true if the file was removed, otherwise axl_false is
 * returned.
 */ 
axl_bool turbulence_unlink              (const char * path)
{
	if (path == NULL)
		return axl_false;

	/* remove the file */
#if defined(AXL_OS_WIN32)
	return (_unlink (path) == 0);
#else
	return (unlink (path) == 0);
#endif
}

/** 
 * @brief Allows to get last modification date for the file provided.
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

/** 
 * @brief Allows to check if the provided file path is a full path
 * (not releative). The function is meant to be portable.
 * 
 * @param file The file to check if it is a full path file name.
 * 
 * @return axl_true if the file is a full path, otherwise axl_false is
 * returned.
 */
int      turbulence_file_is_fullpath (const char * file)
{
	/* check the value received */
	if (file == NULL)
		return axl_false;
#if defined(AXL_OS_UNIX)
	if (file != NULL && (strlen (file) >= 1) && file[0] == '/')
		return axl_true;
#elif defined(AXL_OS_WIN32)
	if (file != NULL && (strlen (file) >= 3) && file[1] == ':' && (file[2] == '/' || file[2] == '\\'))
		return axl_true;
#endif	
	/* the file is not a full path */
	return axl_false;
}


/** 
 * @brief Allows to get the base dir associated to the provided path.
 * 
 * @param path The path that is required to return its base part.
 * 
 * @return Returns the base dir associated to the function. You
 * must deallocate the returning value with axl_free.
 */
char   * turbulence_base_dir            (const char * path)
{
	int    iterator;
	axl_return_val_if_fail (path, NULL);

	/* start with string length */
	iterator = strlen (path) - 1;

	/* lookup for the back-slash */
	while ((iterator >= 0) && 
	       ((path [iterator] != '/') && path [iterator] != '\\'))
		iterator--;

	/* check if the file provided doesn't have any base dir */
	if (iterator == -1) {
		/* return base dir for default location */
		return axl_strdup (".");
	}

	/* check the special case where the base path is / */
	if (iterator == 0 && path [0] == '/')
		iterator = 1;

	/* copy the base dir found */
	return axl_stream_strdup_n (path, iterator);
}

/** 
 * @brief Allows to get the file name associated to the provided path.
 * 
 * @param path The path that is required to return its base value.
 * 
 * @return Returns the base dir associated to the function. You msut
 * deallocate the returning value with axl_free.
 */
char   * turbulence_file_name           (const char * path)
{
	int    iterator;
	axl_return_val_if_fail (path, NULL);

	/* start with string length */
	iterator = strlen (path) - 1;

	/* lookup for the back-slash */
	while ((iterator >= 0) && ((path [iterator] != '/') && (path [iterator] != '\\')))
		iterator--;

	/* check if the file provided doesn't have any file part */
	if (iterator == -1) {
		/* return the an empty file part */
		return axl_strdup (path);
	}

	/* copy the base dir found */
	return axl_strdup (path + iterator + 1);
}

/*
 * @brief Allows to get the next line read from the user. The function
 * return an string allocated.
 * 
 * @return An string allocated or NULL if nothing was received.
 */
char * turbulence_io_get (char * prompt, TurbulenceIoFlags flags)
{
#if defined(ENABLE_TERMIOS)
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
	} /* end while */

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
#else
	return NULL;
#endif
}

/**
 * @brief Allows to get the SYSCONFDIR path provided at compilation
 * time. This is configured when the libturbulence.{dll,so} is built,
 * ensuring all pieces uses the same SYSCONFDIR value. See also \ref
 * turbulence_datadir.
 *
 * The SYSCONFDIR points to the base root directory where all
 * configuration is found. Under unix system it is usually:
 * <b>/etc</b>. On windows system it is usually configured to:
 * <b>../etc</b>. Starting from that directory is found the rest of
 * configurations:
 * 
 *  - etc/turbulence/turbulence.conf
 *  - etc/turbulence/sasl/sasl.conf
 */
const char    * turbulence_sysconfdir     (void)
{
	/* return current configuration */
	return SYSCONFDIR;
}

/** 
 * @brief Allows to get the TBC_DATADIR path provided at compilation
 * time. This is configured when the libturbulence.{dll,so} is built,
 * ensuring all pieces uses the same SYSCONFDIR value. See also \ref
 * turbulence_sysconfdir
 *
 * The TBC_DATADIR points to the base root directory where data files
 * are located (mostly dtd files). Under unix system it is usually:
 * <b>/usr/share/turbulence</b>. On windows system it is usually configured to:
 * <b>../data</b>. Starting from that directory is found the rest of
 * configurations:
 */
const char    * turbulence_datadir        (void)
{
	/* return current configuration */
	return TBC_DATADIR;
}

/** 
 * @brief Allows to check if the provided value contains a decimal
 * number.
 *
 * @param value The decimal number to be checked.
 *
 * @return axl_true in the case a decimal value is found otherwise
 * axl_false is returned.
 */
axl_bool  turbulence_is_num  (const char * value)
{
	int iterator = 0;
	while (iterator < value[iterator]) {
		/* check value on each position */
		if (! isdigit (value[iterator]))
			return axl_false;

		/* next position */
		iterator++;
	}

	/* is a number */
	return axl_true;
}

#if !defined(getpwuid_r)
int getpwuid_r (uid_t uid, struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp);
#endif
#if !defined(getpwnam_r)
int getpwnam_r (const char *name, struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp);
#endif
#if !defined(getgrgid_r)
int getgrgid_r (gid_t gid, struct group *gbuf, char *buf, size_t buflen, struct group **gbufp);
#endif
#if !defined(getgrnam_r)
int getgrnam_r (const char *name, struct group *gbuf, char *buf, size_t buflen, struct group **gbufp);
#endif

/** 
 * @brief Allows to get system user id or system group id from the
 * provided string. 
 *
 * If the string already contains the user id or group id, the
 * function returns its corresponding integet value. The function also
 * checks if the value (that should represent a user or group in some
 * way) is present on the current system. get_user parameter controls
 * if the operation should perform a user lookup or a group lookup.
 * 
 * @param ctx The turbulence context.
 * @param value The user or group to get system id.
 * @param get_user axl_true to signal the value to lookup user, otherwise axl_false to lookup for groups.
 *
 * @return The function returns the user or group id or -1 if it fails.
 */
int turbulence_get_system_id  (TurbulenceCtx * ctx, const char * value, axl_bool get_user)
{
#if defined (AXL_OS_UNIX)
	axl_bool        result;
	struct passwd   user_data;
	struct passwd * user_data_ptr;
	struct group    group_data;
	struct group  * group_data_ptr;
	char            info[1024];

	if (get_user) {
		msg ("checking user id: %s", value);
		memset (&user_data, 0, sizeof (struct passwd));
		if (turbulence_is_num (value)) 
			result = (getpwuid_r (atoi (value), &user_data, info, 1023, &user_data_ptr) == 0);
		else 
			result = (getpwnam_r (value, &user_data, info, 1023, &user_data_ptr) == 0);

		/* check user */
		if (! result) {
			wrn ("Failed to find data associated to user (%s), it seems its missing, errno=%d:%s", value,
			     errno, vortex_errno_get_last_error ());
			CLEAN_START(ctx);
		} /* end if */
		
		/* store user id */
		return (int) user_data.pw_uid;
	} else {
		/* check group */
		msg ("checking group id: %s", value);
		memset (&group_data, 0, sizeof (struct group));
		if (turbulence_is_num (value)) 
			result = (getgrgid_r (atoi (value), &group_data, info, 1023, &group_data_ptr) == 0);
		else 
			result = (getgrnam_r (value, &group_data, info, 1023, &group_data_ptr) == 0);

		/* check user */
		if (! result) {
			wrn ("Failed to find data associated to group (%s), it seems its missing, errno=%d:%s", value,
			     errno, vortex_errno_get_last_error ());
			CLEAN_START(ctx);
		} /* end if */

		/* store group */
		return (int) group_data.gr_gid;
	} /* end if */
#endif
	/* nothing defined */
	return -1;
}

/* horrible hack to get fchown definition */
#if !defined(fchown)
int fchown (int fd, uid_t owner, gid_t group);
#endif

/** 
 * @brief Allows to change the owner (user and group) of the socket's
 * file associated.
 *
 * @param ctx The Turbulence context.
 *
 * @param file_name  The file name path to change its owner.
 *
 * @param user The user string representing the user to change. If no
 * user required to change, just pass empty string or NULL.
 *
 * @param group The group string representing the group to change. If
 * no group required to change, just pass empty string or NULL.
 *
 * @return axl_true if the operation was completed, otherwise false is
 * returned.
 */
axl_bool        turbulence_change_fd_owner (TurbulenceCtx * ctx,
					    const char    * file_name,
					    const char    * user,
					    const char    * group)
{
#if defined(AXL_OS_UNIX)
	/* call to change owners */
	return chown (file_name, 
		       /* get user */
		       turbulence_get_system_id (ctx, user, axl_true), 
		       /* get group */
		       turbulence_get_system_id (ctx, group, axl_false)) == 0;
#endif
	return axl_true;
}

#if !defined(fchmod)
int fchmod(int fildes, mode_t mode);
#endif

/** 
 * @brief Allows to change the permissions of the socket's file associated.
 *
 * @param ctx The Turbulence context.
 *
 * @param file_name The file name path to change perms.
 *
 * @param mode The mode to configure.
 */ 
axl_bool        turbulence_change_fd_perms (TurbulenceCtx * ctx,
					    const char    * file_name,
					    const char    * mode)
{
#if defined(AXL_OS_UNIX)
	mode_t value = strtol (mode, NULL, 8);
	/* call to change mode */
	return chmod (file_name, value) == 0;
#endif
	return axl_true;
}

/* @} */

/** 
 * \mainpage 
 *
 * \section intro Turbulence Documentation
 *
 * The following is the Turbulence API documentation, manuals and
 * other documents comming from modules and tools. 
 *
 * <h2>Turbulence manuals</h2>
 *
 * - \ref turbulence_administrator_manual
 * - \ref turbulence_developer_manual
 *
 * <h2>Vortex API </h2>
 *
 *  Because Turbulence extends and it is built on top of Vortex
 *  Library 1.1, it is required to keep in mind and use Vortex
 *  API. Here is the reference:
 *
 *  - <a class="el" href="http://fact.aspl.es/files/af-arch/vortex-1.1/html/index.html">Vortex Library 1.1 Documentation Center</a>
 *
 * <h2>Turbulence API</h2>
 *
 *  The following is the API exposed to turbulence modules and
 *  tools. This is only useful for turbulence developers.
 *
 *  - \ref turbulence
 *  - \ref turbulence_moddef
 *  - \ref turbulence_config
 *  - \ref turbulence_db_list
 *  - \ref turbulence_conn_mgr
 *  - \ref turbulence_handlers
 *  - \ref turbulence_types
 *  - \ref turbulence_ctx
 *  - \ref turbulence_run
 *  - \ref turbulence_expr
 *  - \ref turbulence_loop
 *  - \ref turbulence_mediator
 */

/** 
 * \page turbulence_administrator_manual Turbulence Administrator manual
 *
 * <b>Section 1: Installation notes</b>
 *
 *   - \ref installing_turbulence 
 *
 * <b>Section 2: Turbulence configuration</b>
 *
 *   - \ref configuring_turbulence
 *   - \ref turbulence_config_location
 *   - \ref turbulence_ports
 *   - \ref turbulence_clean_start
 *   - \ref turbulence_configuring_log_files
 *   - \ref turbulence_db_list_management
 *
 * <b>Section 3: Turbulence module management</b> 
 *
 *   - \ref turbulence_modules_configuration
 *   - \ref turbulence_mod_sasl
 *   - \ref turbulence_mod_tunnel
 *
 * <b>Section 4: BEEP profile management</b>
 *
 *   - \ref profile_path_configuration
 *   - \ref profile_path_flags_supported_by_allow_and_if_sucess
 * 
 * 
 *
 * \section installing_turbulence Installing Turbulence:
 *
 * <b>Turbulence dependencies</b>
 * 
 * Turbulence have the following required dependencies:
 *
 * <ol>
 *  <li> <b>Vortex Library 1.1</b>: which provides the BEEP engine (located at: http://www.aspl.es/vortex).</li>
 *
 *  <li> <b>Axl Library:</b> which provides all XML services (located at: http://www.aspl.es/xml).</li>
 * </ol>
 *
 * The following optional dependencies are not required to built
 * Turbulence, but provides really useful features.
 *
 * <ol> 
 *
 * <li><b>Libpcre3:</b> a library that provides perl expressions
 * support. It is used by all expressions used by the turbulence
 * configuration file. If not provided all expressions provided will
 * be matched as is.
 *
 * This regular expression library was written by Philip Hazel, and
 * copyrighted by the University of Cambridge, England.
 *
 * The software is available at:
 * ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre </li>
 *
 *  <li><b>OpenSSL:</b> a library that provides TLS support. If not
 *  provided the BEEP TLS profile won't be available. This dependency
 *  is required by Vortex Library not by Turbulence.
 *
 *  The software is available at: http://www.openssl.org </li>
 *  
 * <li><b>GNU SASL:</b> a library that provides SASL support. If not
 * provided the BEEP SASL family of profiles won't be available.
 *
 * The software is available at: ftp://alpha.gnu.org/pub/gnu/gsasl </li>
 *
 *</ol>
 *
 * In many cases, GNU SASL, Libpcre3 and OpenSSL are already available
 * on your platform. Check your distribution documentation.  
 *
 *
 * <h3>Building Turbulence from source on POSIX / Unix environments</h3>
 * 
 * It is assumed you have a platform that have autoconf, libtool, and
 * the rest of GNU tools to build software.
 *
 * Get the source code at the download page: http://www.aspl.es/turbulence/downloads.html
 * 
 * Turbulence is compiled following the standard autoconf-compatible
 * procedure:
 *
 * \code
 * >> tar xzvf turbulence-X.X.X-bXXXX.gXXXX.tar.gz
 * >> cd turbulence-X.X.X-bXXXX.gXXXX
 * >> ./configure
 * >> make
 * >> make install
 * \endcode
 *
 * <h3>Once finished, next step</h3>
 *
 * Now you must configure your Turbulence installation. Check the
 * following section.
 * 
 * \section configuring_turbulence Turbulence configuration
 * 
 * \section turbulence_config_location How turbulence is configured (configuration file location)
 *   
 * Turbulence is configured through XML 1.0 files. The intention is
 * provide an easy and extensible way to configure turbulence,
 * allowing third parties to build tools to update/reconfigure it. It
 * is also provided a DTD to ensure that configuration file created is
 * properly constructed at a minimum syntax level.
 * 
 * According to your installation, the main turbulence configuration
 * file should be found at the location provided by the following
 * command: 
 *
 * \include turbulence-config-location.txt
 * 
 * Alternatively you can provide your own configuration file by using
 * the <b>--config</b> option: 
 *
 * \code
 *  >> turbulence --conf my-turbulence.conf
 * \endcode
 *
 * Turbulence main configuration file includes several global
 * sections: 
 *
 * <ol>
 *  <li><b>&lt;global-settings></b>: This main section includes several
 *  run time configuration for the BEEP server: TCP ports, listener
 *  address, log files, crash handling, etc.
 *  </li>
 *
 *  <li>
 *    <b>&lt;modules></b>: Under this section is mainly configured
 * directories holding modules.
 *  </li>
 *
 * <li><b>&lt;profile-path-configuration></b>: Under this section is
 *  provided the profile path configuration, an administrative
 *  configuration which allows to mix, sequence, and select profiles
 *  to be provided to client peers according to several run time
 *  values such remote address, profiles already initiated, etc..
 *  </li>
 *
 * </ol>
 *
 * \section turbulence_ports Turbulence addresses and ports used
 *
 * Ports and addresses used by Turbulence to listen are configured at
 * the <b>&lt;global-settings></b> section. Here is an example:
 * 
 * \htmlinclude global-settings.xml.tmp
 *
 * Previous example will make Turbulence to listen on ports 3206 and
 * 44010 for all addresses that are known for the server hosting
 * turbulence (0.0.0.0). Turbulence will understand this section
 * listening on all addresses provided, for all ports.
 *
 * Then you can use profile path to enforce which profiles will be
 * served on each port. For example, you can provide TUNNEL support
 * only on port 604 (though not recommended: see <a
 * href="beep-applications-and-ports.html">Building wrong port
 * oriented network applications</a>). 
 *
 * \section turbulence_clean_start Debugging turbulence and clean start
 *
 * During turbulence configuration you can use the <b>clean-start</b>
 * feature to ensure your turbulence configuration loads a server with
 * all features requested, failing to start if something goes
 * wrong. This is really useful to ensure a proper startup: for
 * example all ports are allocated. 
 *
 * This is configured inside the <b>global-settings</b> section:
 *
 * \htmlinclude clean-start.xml.tmp
 *
 * 
 * Alternatively, during development or when it is found a turbulence
 * bug, it is handy to configure the default action to take on server
 * crash (bad signal received). This is done by configuring the
 * following:
 *
 * \htmlinclude on-bad-signal.xml.tmp
 *
 * The <b>"hold"</b> option is really useful for real time debugging
 * because it hangs right after the signal is received. This allows to
 * launch the debugger and attach to the process to see what's
 * happening. Inside production environment it is recommended
 * <b>"ignore"</b>.
 *
 *
 * \section turbulence_configuring_log_files Configuring turbulence log files
 *
 * Turbulence, its base library (libturbulence), modules and the tools
 * associated (using previous library) have two destinations where
 * logs are sent. The first default destination for log produced by
 * the turbulence server and its modules are sent to a set of files
 * that are configured at the at the <b>&lt;global-settings></b>
 * section:
 *
 * \htmlinclude log-reporting.xml.tmp
 *
 * These files hold logs for general information
 *  (<b>&lt;general-log></b>), error information
 *  (<b>&lt;error-log></b>), client peer access information
 *  (<b>&lt;access-log></b>) and vortex engine log produced by its
 *  execution (<b>&lt;vortex-log></b>).
 *
 * Apart from the vortex log (<b>&lt;vortex-log></b>) the rest of
 * files contains the information that is produced by all calls done
 * to the following library functions: <b>msg</b>, <b>msg2</b>,
 * <b>wrn</b> and <b>error</b>. 
 *
 * By default, Turbulence server is started with no console
 * output. All log is sent to previous log files. The second
 * destination available is the console output. 
 *
 * Four command line options controls logs produced to the console by
 * Turbulence and tools associated:
 *
 * <ol>
 *  
 * <li><p><b>--debug</b>: activates the console log, showing main
 * messages. Turbulence tools have this option implicitly activated. </p></li>
 *
 * <li><p> <b>--debug2</b>: activates implicitly the <b>--debug</b>
 * option and shows previous messages plus new messages that are
 * considered to have more details.</p></li>
 *
 * <li><p><b>--debug3</b>: makes log output activated to include more
 * details at the place it was launched (file and line), process pid,
 * etc.</p></li>
 *
 * <li><p><b>--color-debug</b>: whenever previous options are activated, if
 * this one is used, the console log is colored according to the
 * kind of message reported: green (messages), red (error messages),
 * yellow (warning messages).</p></li>
 *
 * </ol>
 * 
 * If previous options are used Turbulence will register a log into
 * the appropriate file but also will send the same log to the
 * console.
 *
 * \section turbulence_modules_configuration Turbulence modules configuration
 * 
 * Modules loaded by turbulence are found at the directories
 * configured in the <b>&lt;modules></b> section. Here is an
 * example:
 *
 * \htmlinclude tbc-modules.xml.tmp
 * 
 * Every directory configured contains turbulence xml module pointers
 * having the following content: 
 *
 * \htmlinclude module-conf.xml.tmp
 * 
 * Each module have its own configuration file, which should use XML
 * as default configuration format. 
 * 
 * \section profile_path_configuration Profile path configuration
 *
 * Profile Path is a feature that allows to configure which profiles
 * can be used by remote peers, according to several run time
 * configurations. It is designed to make it easy to develop BEEP
 * profile, that are later mixed with other profiles in many ways
 * making them more useful through the combinations created at
 * run-time.
 * 
 * Let's see some examples to initially clarify the profile path
 * support. A usual configuration around BEEP is to provide SASL
 * authentication and then allow using a profile which do useful
 * work. The intention is to enforce a successful SASL negotiation, to
 * later provide a protected resource. 
 *
 * With profile path this can be configured as:
 * 
 * \htmlinclude path-def.xml.tmp
 *
 * Previous example instruct Turbulence to apply a profile path called
 * "not local-parts" if the source of the connection comes <i>"not
 * from 192.168.0.X"</i>. It also teaches Turbulence to only provide
 * SASL profiles available and only once initiated properly (and
 * authenticated), the remote client peer can use the TUNNEL
 * profile. 
 *
 * Profile path is applied as a chain, composed by a set of
 * <b>&lt;path-def></b> declarations. Once a <b>&lt;path-def></b>
 * match the target, the profile path configuration found on it is
 * applied to the peer, discarding the rest of <b>&lt;path-def></b>
 * defined.  
 * 
 * \image html profile-path-chain.png "Profile path chain"
 *
 * As the previous image shows, the turbulence profile path
 * configuration is composed by several profile path definitions: 
 * 
 * \htmlinclude path-def-conf.xml.tmp
 * 
 * Once a connection is received, a path-def is selected and the
 * configuration inside it is applied. If no path-def is selected, the
 * connection is rejected.
 *
 * Every <b>&lt;path-def></b> supports the following matching configurations:
 *
 * <ol> 
 * <li><p><b>server-name</b>: a perl expression defining the serverName
 * to match for the connection. This can be used to only provide
 * services based on a virtual hosting.</p></li>
 *
 * <li><p><b>src</b>: a perl expression defining the source of the
 * connection. This can be used to only provide critical services to
 * local administrators.</p></li>
 *
 * <li><p><b>path-name</b>: an administrative flag that will help to
 * recognize the connection in future process (log
 * reporting).</p></li>
 * 
 * </ol> 
 * 
 * Once a path is matched, the following are discarded and the
 * configuration inside the profile path is applied to the connection
 * as long as the connection is running.
 * 
 * Now, it is required to configure which profiles will be
 * allowed. This is done by using two types of nodes:
 *
 * <ol>
 * 
 * <li><p><b>&lt;if-success></b>: a configuration that allows to use
 * the profile referenced by it, and additionally, allows to use
 * profiles provided by its child nodes once a channel with the
 * profile provided is created.</p></li>
 *
 * <li><p><b>&lt;allow></b>: a configuration that allows to use a
 * profile at a particular level. </p></li>
 *
 * </ol>
 *
 * Let's see an example to show how it works:
 * 
 * \htmlinclude path-def-example.xml.tmp
 *
 * Previous example have configured a particular profile path as
 * follows: 
 *
 * <div class="center"><img src="images/profile-path-example.png" alt="[PROFILE PATH EXAMPLE]"></div>
 *
 * The example is mostly self-explanatory, but one detail remains. The
 * caonfiguration uses two attributes: <b>connmark</b> and
 * <b>preconnmark</b>. They are used as flags that must be detected in
 * the connection in other to allow the connection to accept a profile
 * or the content of the following profiles.
 *
 * \section profile_path_flags_supported_by_allow_and_if_sucess Profile path configuration: flags supported by <allow> and <if-success>
 *
 * The following are the flags supported by <b>&lt;allow></b> and
 * <b>&lt;if-success></b>:
 *
 * <ol>
 *
 * <li><p><b>preconnmark</b>: if used, it means that the connection
 * must have the "mark" provided before the profile can be accepted to
 * be initiated. <br>SUPPORTED: &lt;allow>, &lt;if-success></p></li>
 * 
 * <li><p><b>connmark</b>: can only be used from &lt;if-success> nodes, and
 * allows to protect the profile path content inside the
 * &lt;if-success> node, enforcing to not only create a channel under
 * the profile provided but also having the mark. <br>SUPPORTED:
 * &lt;if-success></p>
 *
 *
 * <p>This is particular useful for the SASL case because having a
 * SASL channel created isn't a warranty of a connection
 * authenticated. Thus, an additional mark is required to properly
 * ensure that the connection was authenticated.</p></li>
 *
 * <li><p><b>max-per-con</b>: allows to configure the maximum amount
 * of channels running the profile provided in the particular
 * connection instance. <br>SUPPORTED: &lt;allow>,
 * &lt;if-success></p></li>
 *
 * </ol>
 * 
 * <p>So, a good question at this point is "how are those marks
 * created?". These marks are profile dependant and are created using
 * the interface provided by the vortex connection module to store
 * data. You must check the profile documentation to know which marks
 * are supported as they are particular to each BEEP profile. </p>
 * 
 * <p>In this case, Vortex Library SASL implementation flags the
 * connection as authenticated using the provided flag:
 * "sasl:is:authenticated".</p>
 *
 */

/** 
 * \page turbulence_developer_manual Turbulence Developer manual
 *
 * <b>Section 1: Creating turbulence modules</b>
 *
 *   - \ref turbulence_developer_manual_creating_modules
 *
 * \section turbulence_developer_manual_creating_modules How Turbulence module works
 *
 * Turbulence, from a simple point of view, is a listener application
 * built on top of <a href="http://www.aspl.es/vortex">Vortex
 * Library</a>, which reads a set of \ref configuring_turbulence
 * "configuration files" to start at some selected ports, etc, and
 * then load all modules installed to implement useful BEEP based
 * things.
 *
 * These modules could implement new BEEP profiles or features that
 * extend Turbulence internal function (not necessarily a new BEEP
 * profile). This is because module structure is really simple (and
 * this is intentional).
 *
 * Turbulence core is really small. The rest of features are added as
 * modules. For example, Turbulence SASL support is a module which is
 * configurable to use a particular user database and, with the help
 * of some tools (<b>tbc-sasl-conf</b>), you can manage users that can
 * connect to Turbulence.
 *
 * In fact, Turbulence doesn't known anything about SASL. \ref
 * turbulence_mod_sasl "Turbulence SASL module" is implemented to
 * install and configure the SASL profiles provided by Vortex, and
 * using \ref profile_path_configuration "Profile Path" (a Turbulence
 * core feature), the security provisioning is meet.
 *
 * Turbulence module form is fairly simple. It contains the following handlers (defined at \ref TurbulenceModDef):
 * <ol>
 *
 *  <li>Init (\ref ModInitFunc): A handler called by Turbulence to start the module. Here
 *  the developer must place all calls required to install/configure a
 *  profile, init global variables, etc.</li>
 *
 *  <li>Close (\ref ModCloseFunc): Called by Turbulence to stop a module. Here the
 *  developer must stop and dealloc all resources used by its
 *  module.</li>
 *
 *  <li>Reconf (\ref ModReconfFunc): Called by Turbulence when a HUP signal is
 *  received. This is a notification that the module should reload its
 *  configuration files and start to behave as they propose.</li>
 *
 * </ol>
 *
 * \section turbulence_developer_manual_creating_modules_manually Creating a module from the scratch (dirty way)
 *
 * Maybe the easiest way to start writing a Turbulence Module is to
 * take a look into mod-test source code. This module doesn't do
 * anything but is maintained across releases to contain all handlers
 * required and a brief help. You can use it as an official
 * reference. A module is at minimum composed by the following tree
 * files:
 *
 * - <b>mod-test.c</b>: base module source code: \ref turbulence_mod_test_c "mod-test.c" | <a href="https://dolphin.aspl.es/svn/publico/af-arch/turbulence/modules/mod-test/mod-test.c"><b>[TXT]</b></a>
 * - <b>Makefile.am</b>: optional automake file used to build the module: <a href="https://dolphin.aspl.es/svn/publico/af-arch/turbulence/modules/mod-test/Makefile.am"><b>[TXT]</b></a>
 * - <b>mod-test.xml.in</b>: xml module pointer, a file that is installed at the Turbulence modules dir to load the module: <a href="https://dolphin.aspl.es/svn/publico/af-arch/turbulence/modules/mod-test/mod-test.xml.in"><b>[TXT]</b></a>
 *
 * Now if your intention is to built a BEEP profile then you should do
 * all calls to install it and its associated handlers using the
 * vortex profiles API at the Init (\ref ModInitFunc) handler.
 */

/** 
 * \page turbulence_mod_test_c mod-test.c source code
 *
 * You can copy and paste the following code to start a turbulence module. This code is checked against Turbulence source code.
 *
 * \include mod-test.c
 */



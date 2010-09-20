/*  Turbulence BEEP application server
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
#ifndef __TURBULENCE_H__
#define __TURBULENCE_H__

/* system includes */
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>

/* BEEP support */
#include <vortex.h>

/* XML support */
#include <axl.h>

/* local includes */
#include <turbulence-types.h>
#include <turbulence-ctx.h>
#include <turbulence-handlers.h>
#include <turbulence-expr.h>
#include <turbulence-support.h>
#include <turbulence-signal.h>
#include <turbulence-moddef.h>
#include <turbulence-config.h>
#include <turbulence-run.h>
#include <turbulence-module.h>
#include <turbulence-log.h>
#include <turbulence-ppath.h>
#include <turbulence-db-list.h>
#include <turbulence-conn-mgr.h>
#include <turbulence-process.h>
#include <turbulence-loop.h>
#include <turbulence-mediator.h>

/** 
 * \addtogroup turbulence
 * @{
 */

axl_bool  turbulence_log_enabled      (TurbulenceCtx * ctx);

void      turbulence_log_enable       (TurbulenceCtx * ctx, 
				       int  value);

axl_bool  turbulence_log2_enabled     (TurbulenceCtx * ctx);

void      turbulence_log2_enable      (TurbulenceCtx * ctx,
				       int  value);

axl_bool  turbulence_log3_enabled     (TurbulenceCtx * ctx);

void      turbulence_log3_enable      (TurbulenceCtx * ctx,
				       int  value);

void      turbulence_color_log_enable (TurbulenceCtx * ctx,
				       int             value);

/** 
 * Drop an error msg to the console stderr.
 *
 * To drop an error message use:
 * \code
 *   error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define error(m,...) do{turbulence_error (ctx, axl_false, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_error (TurbulenceCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...);

/** 
 * Drop an error msg to the console stderr without taking into
 * consideration debug configuration.
 *
 * To drop an error message use:
 * \code
 *   abort_error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define abort_error(m,...) do{turbulence_error (ctx, axl_true, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)

/** 
 * Drop a msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg(m,...)   do{turbulence_msg (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_msg   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drop a second level msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg2 ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg2(m,...)   do{turbulence_msg2 (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_msg2   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...);



/** 
 * Drop a warning msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   wrn ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn(m,...)   do{turbulence_wrn (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_wrn   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drops to the console stdout a warning, placing the content prefixed
 * with the file and the line that caused the message, without
 * introducing a new line.
 *
 * To drop a message use:
 * \code
 *   wrn_sl ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn_sl(m,...)   do{turbulence_wrn_sl (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_wrn_sl   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Reports an access message, a message that is sent to the access log
 * file. The message must contain access to the server information.
 *
 * To drop a message use:
 * \code
 *   access ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define access(m,...)   do{turbulence_access (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  turbulence_access   (TurbulenceCtx * ctx, const char * file, int line, const char * format, ...);

int      turbulence_init (TurbulenceCtx * ctx, 
			  VortexCtx     * vortex_ctx,
			  const char    * config);

void     turbulence_exit                (TurbulenceCtx * ctx,
					 int             free_ctx,
					 int             free_vortex_ctx);

void     turbulence_reload_config       (TurbulenceCtx * ctx, int value);

axl_bool turbulence_file_test_v         (const char * format, 
					 VortexFileTest test, ...);

axl_bool turbulence_create_dir          (const char * path);

axl_bool turbulence_unlink              (const char * path);

long     turbulence_last_modification   (const char * file);

axl_bool turbulence_file_is_fullpath    (const char * file);

char   * turbulence_base_dir            (const char * path);

char   * turbulence_file_name           (const char * path);

typedef enum {
	DISABLE_STDIN_ECHO = 1 << 0,
} TurbulenceIoFlags;

char *   turbulence_io_get (char * prompt, TurbulenceIoFlags flags);

/* enviroment configuration */
const char    * turbulence_sysconfdir      (TurbulenceCtx * ctx);

const char    * turbulence_datadir         (TurbulenceCtx * ctx);

const char    * turbulence_runtime_datadir (TurbulenceCtx * ctx);

axl_bool        turbulence_is_num         (const char * value);

int             turbulence_get_system_id  (TurbulenceCtx * ctx,
					   const char    * value, 
					   axl_bool        get_user);

axl_bool        turbulence_change_fd_owner (TurbulenceCtx * ctx,
					    const char    * file_name,
					    const char    * user,
					    const char    * group);

axl_bool        turbulence_change_fd_perms (TurbulenceCtx * ctx,
					    const char    * file_name,
					    const char    * mode);

void            turbulence_sleep           (TurbulenceCtx * ctx,
					    long            microseconds);

#endif

/* @} */

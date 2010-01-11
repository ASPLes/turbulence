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
#ifndef __TURBULENCE_PROCESS_H__
#define __TURBULENCE_PROCESS_H__

#include <turbulence.h>

void              turbulence_process_init         (TurbulenceCtx * ctx, 
						   axl_bool        reinit);

void              turbulence_process_create_child (TurbulenceCtx       * ctx, 
						   VortexConnection    * conn,
						   TurbulencePPathDef  * def,
						   axl_bool              handle_start_reply,
						   int                   channel_num,
						   const char          * profile,
						   const char          * profile_content,
						   VortexEncoding        encoding,
						   const char          * serverName,
						   VortexFrame         * frame);

void              turbulence_process_kill_childs  (TurbulenceCtx * ctx);

int               turbulence_process_child_count  (TurbulenceCtx * ctx);

axl_bool          turbulence_process_child_exits  (TurbulenceCtx * ctx, int pid);

int               turbulence_process_find_pid_from_ppath_id (TurbulenceCtx * ctx, int pid);

TurbulenceChild * turbulence_process_get_child_from_ppath (TurbulenceCtx      * ctx, 
							   TurbulencePPathDef * def,
							   axl_bool             acquire_mutex);

void              turbulence_process_cleanup      (TurbulenceCtx * ctx);

/* internal API */
char *           turbulence_process_connection_status_string (axl_bool          handle_start_reply,
							      int               channel_num,
							      const char      * profile,
							      const char      * profile_content,
							      VortexEncoding    encoding,
							      const char      * serverName,
							      int               msg_no,
							      int               seq_no,
							      int               seq_no_expected);

void             turbulence_process_connection_recover_status (char            * ancillary_data,
							       axl_bool        * handle_start_reply,
							       int             * channel_num,
							       const char     ** profile,
							       const char     ** profile_content,
							       VortexEncoding  * encoding,
							       const char     ** serverName,
							       int             * msg_no,
							       int             * seq_no,
							       int             * seq_no_expected);

#endif
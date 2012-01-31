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
#ifndef __TURBULENCE_RUN_H__
#define __TURBULENCE_RUN_H__

#include <turbulence.h>

/** 
 * \addtogroup turbulence_run
 * @{
 */

int  turbulence_run_config    (TurbulenceCtx * ctx);

void turbulence_run_cleanup   (TurbulenceCtx * ctx);

/** 
 * @brief Shutdown and closes the connection.
 * @param conn The connection to shutdown and close.
 */
#define TBC_FAST_CLOSE(conn) do{	                                           \
	error ("shutdowing connection id=%d..", vortex_connection_get_id (conn));  \
        vortex_connection_set_close_socket (conn, axl_true);                       \
	vortex_connection_shutdown (conn);                                         \
	vortex_connection_close    (conn);                                         \
        conn = NULL;                                                               \
	} while (0);

axl_bool turbulence_run_check_no_load_module (TurbulenceCtx * ctx, 
					      const char    * module_to_check);

/** 
 * @}
 */

#endif

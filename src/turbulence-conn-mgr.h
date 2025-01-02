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
#ifndef __TURBULENCE_CONN_MGR_H__
#define __TURBULENCE_CONN_MGR_H__

/* internal use API */
void turbulence_conn_mgr_init          (TurbulenceCtx    * ctx, 
					axl_bool           reinit);

void turbulence_conn_mgr_register      (TurbulenceCtx    * ctx, 
					VortexConnection * conn);

void turbulence_conn_mgr_unregister    (TurbulenceCtx    * ctx, 
					VortexConnection * conn);

void turbulence_conn_mgr_cleanup       (TurbulenceCtx * ctx);

/* public API */
axl_bool  turbulence_conn_mgr_broadcast_msg (TurbulenceCtx            * ctx,
					     const void               * message,
					     int                        message_size,
					     const char               * profile,
					     TurbulenceConnMgrFilter    filter_conn,
					     axlPointer                 filter_data);

axlList *  turbulence_conn_mgr_conn_list   (TurbulenceCtx            * ctx, 
					    VortexPeerRole             role,
					    const char               * filter);

axlList *  turbulence_conn_mgr_conn_list   (TurbulenceCtx            * ctx, 
					    VortexPeerRole             role,
					    const char               * filter);

int        turbulence_conn_mgr_count       (TurbulenceCtx            * ctx);

axl_bool   turbulence_conn_mgr_proxy_on_parent (VortexConnection * conn);

int        turbulence_conn_mgr_setup_proxy_on_parent (TurbulenceCtx * ctx, VortexConnection * conn);

VortexConnection * turbulence_conn_mgr_find_by_id (TurbulenceCtx * ctx,
						   int             conn_id);

axlHashCursor    * turbulence_conn_mgr_profiles_stats (TurbulenceCtx    * ctx,
						       VortexConnection * conn);



/* private API */
void turbulence_conn_mgr_on_close (VortexConnection * conn, 
				   axlPointer         user_data);


#endif 

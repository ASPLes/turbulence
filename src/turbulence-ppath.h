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
#ifndef __TURBULENCE_PPATH_H__
#define __TURBULENCE_PPATH_H__

#include <turbulence.h>

int  turbulence_ppath_init (TurbulenceCtx * ctx);

void turbulence_ppath_cleanup (TurbulenceCtx * ctx);

void turbulence_ppath_change_user_id (TurbulenceCtx      * ctx, 
				      TurbulencePPathDef * ppath_def);

void turbulence_ppath_change_root    (TurbulenceCtx      * ctx, 
				      TurbulencePPathDef * ppath_def);

TurbulencePPathDef * turbulence_ppath_selected (VortexConnection * conn);

int                  turbulence_ppath_get_id   (TurbulencePPathDef * ppath_def);

const char         * turbulence_ppath_get_name (TurbulencePPathDef * ppath_def);

const char         * turbulence_ppath_get_work_dir    (TurbulenceCtx      * ctx,
						       TurbulencePPathDef * ppath_def);

const char         * turbulence_ppath_get_server_name (VortexConnection * conn);

axl_bool __turbulence_ppath_select   (TurbulenceCtx      * ctx, 
				      VortexConnection   * connection, 
				      int                  channel_num,
				      const char         * uri,
				      const char         * profile_content,
				      VortexEncoding       encoding,
				      const char         * serverName, 
				      VortexFrame        * frame,
				      axl_bool             on_connect);

#endif /* end __TURBULENCE_PPATH_H__ */

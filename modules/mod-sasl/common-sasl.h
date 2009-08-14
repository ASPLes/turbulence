/*
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
#ifndef __COMMON_SASL_H__
#define __COMMON_SASL_H__

/* turbulence files */
#include <turbulence.h>

/* include sasl support */
#include <vortex_sasl.h>

/* include tls support */
#include <vortex_tls.h>

/* xml-rpc types */
#include <sasl_radmin_types.h>

/** 
 * @brief Public type representing a SASL backend loaded.
 */
typedef struct _SaslAuthBackend SaslAuthBackend;

typedef struct _SaslAuthDb SaslAuthDb;

int             common_sasl_load_config    (TurbulenceCtx    * ctx,
					    SaslAuthBackend ** sasl_backend,
					    const char       * alt_location,
					    VortexMutex      * mutex);

int             common_sasl_auth_user      (SaslAuthBackend  * sasl_backend,
					    VortexConnection * conn,
					    const char       * auth_id,
					    const char       * authorization_id,
					    const char       * password,
					    const char       * serverName,
					    VortexMutex      * mutex);

int             common_sasl_method_allowed (SaslAuthBackend  * sasl_backend,
					    const char       * sasl_method,
					    VortexMutex      * mutex);

int             common_sasl_user_exists    (SaslAuthBackend   * sasl_backend,
					    const char        * auth_id,
					    const char        * serverName,
					    axlError         ** err,
					    VortexMutex       * mutex);

int             common_sasl_serverName_exists (SaslAuthBackend   * sasl_backend,
					       const char        * serverName,
					       axlError         ** err,
					       VortexMutex       * mutex);

int             common_sasl_user_add       (SaslAuthBackend  * sasl_backend, 
					    const char       * auth_id, 
					    const char       * password, 
					    const char       * serverName, 
					    VortexMutex      * mutex);

int             common_sasl_user_password_change (SaslAuthBackend * sasl_backend,
						  const char      * auth_id,
						  const char      * new_password,
						  const char      * serverName,
						  VortexMutex     * mutex);

int             common_sasl_user_edit_auth_id       (SaslAuthBackend  * sasl_backend, 
						     const char       * auth_id, 
						     const char       * new_auth_id,
						     const char       * serverName, 
						     VortexMutex      * mutex);

int             common_sasl_user_disable      (SaslAuthBackend  * sasl_backend, 
					       const char       * auth_id, 
					       const char       * serverName,
					       int                disable,
					       VortexMutex      * mutex);

int             common_sasl_user_is_disabled (SaslAuthBackend  * sasl_backend,
					      const char       * auth_id, 
					      const char       * serverName,
					      VortexMutex      * mutex);

int             common_sasl_enable_remote_admin  (SaslAuthBackend  * sasl_backend, 
						  const char       * auth_id, 
						  const char       * serverName,
						  int                enable,
						  VortexMutex      * mutex);

int             common_sasl_is_remote_admin_enabled (SaslAuthBackend  * sasl_backend,
						     const char       * auth_id, 
						     const char       * serverName,
						     VortexMutex      * mutex);
					

axlList *       common_sasl_get_users      (SaslAuthBackend  * sasl_backend,
					    const char       * serverName,
					    VortexMutex      * mutex);

int             common_sasl_user_remove    (SaslAuthBackend  * sasl_backend,
					    const char       * auth_id, 
					    const char       * serverName, 
					    VortexMutex      * mutex);

TurbulenceCtx * common_sasl_get_context    (SaslAuthBackend * backend);

void            common_sasl_free_common    (SaslAuthBackend * backend, axl_bool dump_content);
void            common_sasl_free           (SaslAuthBackend  * backend);



/* remote interface API */
int       common_sasl_activate_remote_admin (SaslAuthBackend * sasl_backend,
					     VortexMutex     * mutex);

int       common_sasl_validate_resource (VortexConnection * conn,
					 int                channel_num,
					 const char       * serverName,
					 const char       * resource_path,
					 axlPointer         user_data);



/* private API */
int  common_sasl_load_auth_db_xml (SaslAuthBackend  * sasl_backend,
				   axlNode          * node,
				   VortexMutex      * mutex);
				   
int  common_sasl_load_users_db    (TurbulenceCtx    * ctx,
				   SaslAuthDb       * db,
				   VortexMutex      * mutex);

#endif

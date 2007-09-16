/*
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
#ifndef __COMMON_SASL_H__
#define __COMMON_SASL_H__

#include <turbulence.h>

/** 
 * @brief Public type representing a SASL backend loaded.
 */
typedef struct _SaslAuthBackend SaslAuthBackend;

typedef struct _SaslAuthDb SaslAuthDb;

/** 
 * @brief Public structure representing a user found in the
 * database. This structure is mainly used by the function \ref
 * common_sasl_get_users function.
 */
typedef struct _SaslUser {
	/** 
	 * @brief Auth id for the user stored user.
	 */
	char * auth_id;
	
	/** 
	 * @brief Flags if the user was disabled.
	 */
	bool   disabled;
} SaslUser;

bool      common_sasl_load_config    (SaslAuthBackend ** sasl_backend,
				      const char       * alt_location,
				      VortexMutex      * mutex);

bool      common_sasl_auth_user      (SaslAuthBackend  * sasl_backend,
				      const char       * auth_id,
				      const char       * authorization_id,
				      const char       * password,
				      const char       * serverName,
				      VortexMutex      * mutex);

bool      common_sasl_method_allowed (SaslAuthBackend  * sasl_backend,
				      const char       * sasl_method,
				      VortexMutex      * mutex);

bool      common_sasl_user_exists    (SaslAuthBackend   * sasl_backend,
				      const char        * auth_id,
				      const char        * serverName,
				      axlError         ** err,
				      VortexMutex       * mutex);

bool      common_sasl_user_add       (SaslAuthBackend  * sasl_backend, 
				      const char       * auth_id, 
				      const char       * password, 
				      const char       * serverName, 
				      VortexMutex      * mutex);

bool      common_sasl_user_disable      (SaslAuthBackend  * sasl_backend, 
					 const char       * auth_id, 
					 const char       * serverName,
					 bool               disable,
					 VortexMutex      * mutex);

bool      common_sasl_user_is_disabled (SaslAuthBackend  * sasl_backend,
					const char       * auth_id, 
					const char       * serverName,
					VortexMutex      * mutex);
					

axlList * common_sasl_get_users      (SaslAuthBackend  * sasl_backend,
				      const char       * serverName,
				      VortexMutex      * mutex);

bool      common_sasl_user_remove    (SaslAuthBackend  * sasl_backend,
				      const char       * auth_id, 
				      const char       * serverName, 
				      VortexMutex      * mutex);

void      common_sasl_free           (SaslAuthBackend  * backend);

/* remote interface API */
bool      common_sasl_activate_remote_admin (SaslAuthBackend * sasl_backend,
					     VortexMutex     * mutex);

bool      common_sasl_validate_resource (VortexConnection * conn,
					 int                channel_num,
					 const char       * serverName,
					 const char       * resource_path,
					 axlPointer         user_data);



/* private API */
bool common_sasl_load_auth_db_xml (SaslAuthBackend  * sasl_backend,
				   axlNode          * node,
				   VortexMutex      * mutex);
				   
bool common_sasl_load_users_db    (SaslAuthDb       * db,
				   VortexMutex      * mutex);

#endif

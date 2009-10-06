/*  Turbulence:  BEEP application server
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
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
#ifndef __TURBULENCE_MEDIATOR_H__
#define __TURBULENCE_MEDIATOR_H__

/** 
 * \addtogroup turbulence_mediator
 * @{
 */

#include <turbulence.h>

/** 
 * @brief Event notification object. This type contains all data
 * associated to an event registered at the turbulence mediator API.
 */
typedef struct _TurbulenceMediatorObject TurbulenceMediatorObject;

/** 
 * @brief Handler definition for the set of functions called to get a
 * notification that at registered event have ocurred. The set of data
 * notified by the event is hold by the object instance received (\ref
 * TurbulenceMediatorObject).
 *
 * @param object This parameter contains all data notified by the particular event.
 *
 * @param user_data This is a user defined pointer (defined at \ref
 * turbulence_mediator_create_plug or \ref
 * turbulence_mediator_subscribe).
 */
typedef void (*TurbulenceMediatorHandler) (TurbulenceMediatorObject * object);

void turbulence_mediator_init         (TurbulenceCtx * ctx);

/** 
 * @brief Enum definition used by \ref turbulence_mediator_object_get
 * that allows to retrieve the particular data required from the
 * event.
 */
typedef enum {
	/** 
	 * @brief Ask object to return the \ref TurbulenceCtx associated.
	 */
	TURBULENCE_MEDIATOR_ATTR_CTX             = 1,
	/** 
	 * @brief Ask object to return the event name.
	 */
	TURBULENCE_MEDIATOR_ATTR_ENTRY_NAME      = 2,
	/** 
	 * @brief Ask object to return the event domain.
	 */
	TURBULENCE_MEDIATOR_ATTR_ENTRY_DOMAIN    = 3,
	/** 
	 * @brief User defined pointer that was configured at \ref
	 * turbulence_mediator_create_plug or \ref
	 * turbulence_mediator_subscribe.
	 */
	TURBULENCE_MEDIATOR_ATTR_USER_DATA       = 4,
	/** 
	 * @brief Ask object to return first user data defined. Note
	 * this is event specific so you have to check event
	 * documentation.
	 */
	TURBULENCE_MEDIATOR_ATTR_EVENT_DATA      = 5,
	/** 
	 * @brief Ask object to return second user data defined. Note
	 * this is event specific so you have to check event
	 * documentation.
	 */
	TURBULENCE_MEDIATOR_ATTR_EVENT_DATA2     = 6,
	/** 
	 * @brief Ask object to return thid user data defined. Note
	 * this is event specific so you have to check event
	 * documentation.
	 */
	TURBULENCE_MEDIATOR_ATTR_EVENT_DATA3     = 7,
	/** 
	 * @brief Ask object to return thid user data defined. Note
	 * this is event specific so you have to check event
	 * documentation.
	 */
	TURBULENCE_MEDIATOR_ATTR_EVENT_DATA4     = 8
} TurbulenceMediatorAttr;

axlPointer  turbulence_mediator_object_get (TurbulenceMediatorObject * object,
					    TurbulenceMediatorAttr     attr);

void        turbulence_mediator_object_set_result (TurbulenceMediatorObject * object,
						   axlPointer                 result);

axl_bool turbulence_mediator_create_plug  (TurbulenceCtx             * ctx,
					   const char                * entry_name, 
					   const char                * entry_domain,
					   axl_bool                    subscribe,
					   TurbulenceMediatorHandler   handler, 
					   axlPointer                  user_data);

int      turbulence_mediator_plug_num     (TurbulenceCtx             * ctx);

axl_bool turbulence_mediator_plug_exits   (TurbulenceCtx             * ctx,
					   const char                * entry_name,
					   const char                * entry_domain);

axl_bool turbulence_mediator_subscribe    (TurbulenceCtx             * ctx,
					   const char                * entry_name,
					   const char                * entry_domain,
					   TurbulenceMediatorHandler   handler,
					   axlPointer                  user_data);

axl_bool turbulence_mediator_create_api   (TurbulenceCtx             * ctx,
					   const char                * entry_name, 
					   const char                * entry_domain,
					   TurbulenceMediatorHandler   handler, 
					   axlPointer                  user_data);

void     turbulence_mediator_remove_plug  (TurbulenceCtx             * ctx,
					   const char                * entry_name,
					   const char                * entry_domain,
					   TurbulenceMediatorHandler   handler,
					   axlPointer                  user_data);

void     turbulence_mediator_push_event   (TurbulenceCtx             * ctx,
					   const char                * entry_name,
					   const char                * entry_domain,
					   axlPointer                  event_data,
					   axlPointer                  event_data2,
					   axlPointer                  event_data3,
					   axlPointer                  event_data4);

axlPointer     turbulence_mediator_call_api     (TurbulenceCtx             * ctx,
						 const char                * entry_name,
						 const char                * entry_domain,
						 axlPointer                  event_data,
						 axlPointer                  event_data2,
						 axlPointer                  event_data3,
						 axlPointer                  event_data4);

void     turbulence_mediator_cleanup      (TurbulenceCtx * ctx);

#endif

/** 
 * @}
 */ 

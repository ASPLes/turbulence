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
#include <turbulence.h>

/* include private ctx */
#include <turbulence-ctx-private.h>

struct _TurbulenceMediatorObject {
	TurbulenceCtx * ctx;
	const char    * entry_name;
	const char    * entry_domain;
	axlPointer      user_data;
	axlPointer      event_data;
	axlPointer      event_data2;
	axlPointer      event_data3;
};

/** 
 * @internal API used to initialize mediator module.
 * @param The turbulence context where the mediator will be initialized.
 */
void turbulence_mediator_init        (TurbulenceCtx * ctx)
{
	/* init mutex */
	vortex_mutex_create (&ctx->mediator_hash_mutex);
	/* init hash */
	if (ctx->mediator_hash == NULL)
		ctx->mediator_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	return;
}

/** 
 * @brief Allows to retrieve the particular attribute value from the
 * mediator object (\ref TurbulenceMediatorObject).
 *
 * @param object The mediator object being checked for one of its attributes.
 *
 * @param attr The particular attribute being check. Check \ref
 * TurbulenceMediatorAttr to know about attributes available.
 *
 * @return The function returns the value associated to the
 * attribute. Keep in mind some attributes may return a boolea value,
 * others may return an string or a pointer reference. The value
 * returned is entirely event specific. Check its documentation. The
 * function may return NULL either because NULL value is defined on
 * the attribute or because a NULL object was received.
 */
axlPointer  turbulence_mediator_object_get (TurbulenceMediatorObject * object,
					    TurbulenceMediatorAttr     attr)
{
	if (object == NULL)
		return NULL;
	/* get the particular value associated to the entry name */
	switch (attr) {
	case TURBULENCE_MEDIATOR_ATTR_CTX:
		return (axlPointer)object->ctx;
	case TURBULENCE_MEDIATOR_ATTR_ENTRY_NAME:
		return (axlPointer)object->entry_name;
	case TURBULENCE_MEDIATOR_ATTR_ENTRY_DOMAIN:
		return (axlPointer)object->entry_domain;
	case TURBULENCE_MEDIATOR_ATTR_USER_DATA:
		return (axlPointer)object->user_data;
	case TURBULENCE_MEDIATOR_ATTR_EVENT_DATA:
		return (axlPointer)object->event_data;
	case TURBULENCE_MEDIATOR_ATTR_EVENT_DATA2:
		return (axlPointer)object->event_data2;
	case TURBULENCE_MEDIATOR_ATTR_EVENT_DATA3:
		return (axlPointer)object->event_data3;
	}
	/* requested an unsupported value */
	return NULL;
}

/** 
 * @brief Allows to create a new event plug, a notification bridge
 * where turbulence modules can subscribe and push events allowing
 * component collaboration without having direct connection between
 * them (low coupling).
 *
 * This function is used to create a new event and can be used to
 * register in the same step (setting subscribe to axl_true).
 *
 * @param entry_name This is the event entry name.
 *
 * @param entry_domain This is the event entry domain, an string that
 * can be used to group together events with similiar or connected
 * logic.
 *
 * @param subscribe Set to axl_true to subscribe to the event (as \ref
 * turbulence_mediator_subscribe) using the handler and user data
 * provided.
 *
 * @param handler The handler to be called in the case an event happens.
 *
 * @param user_data User defined pointer passed to the handler when
 * the event happens.
 */
void turbulence_mediator_create_plug (const char                * entry_name, 
				      const char                * entry_domain,
				      axl_bool                    subscribe,
				      TurbulenceMediatorHandler   handler, 
				      axlPointer                  user_data)
{
	
	return;
}

void turbulence_mediator_subscribe   (const char                * entry_name,
				      const char                * entry_domain,
				      TurbulenceMediatorHandler   handler,
				      axlPointer                  user_data)
{
	return;
}

void turbulence_mediator_remove_plug (const char                * entry_name,
				      const char                * entry_domain,
				      TurbulenceMediatorHandler   handler,
				      axlPointer                  user_data)
{
	return;
}


void turbulence_mediator_push_event  (const char                * entry_name,
				      const char                * entry_domain,
				      axlPointer                  user_data,
				      axlPointer                  user_data2,
				      axlPointer                  user_data3)
{
	return;
}


/** 
 * @internal API used by turbulence to terminate mediator module
 * function.
 */
void turbulence_mediator_cleanup      (TurbulenceCtx * ctx)
{
	if (ctx == NULL || ctx->mediator_hash == NULL)
		return;

	/* finish hash */
	axl_hash_free (ctx->mediator_hash);

	/* clear mutex */
	vortex_mutex_destroy (&ctx->mediator_hash_mutex);
	

	return;
}


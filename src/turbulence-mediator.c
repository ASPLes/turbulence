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
#include <turbulence.h>

/** 
 * \defgroup turbulence_mediator Turbulence Mediator: broker API used to communicate modules and turbulence components
 */

/** 
 * \addtogroup turbulence_mediator
 * @{
 */

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
	axlPointer      event_data4;
	/* result returned for api calls */
	axlPointer      result;
};

typedef struct _TurbulenceMediatorPlug {
	axl_bool                       is_api;
 	char                         * entry_name;
	char                         * entry_domain;
	axlList                      * subscribers;
	TurbulenceMediatorHandler      api_handler;
	axlPointer                     user_data;
} TurbulenceMediatorPlug;

typedef struct _TurbulenceMediatorSubscriber {
	TurbulenceMediatorHandler handler;
	axlPointer                user_data;
} TurbulenceMediatorSubscriber;

/** 
 * @internal API used to initialize mediator module.
 * @param The turbulence context where the mediator will be initialized.
 */
void turbulence_mediator_init        (TurbulenceCtx * ctx)
{
	/* init mutex */
	vortex_mutex_create (&ctx->mediator_hash_mutex);
	/* init hash */
	if (ctx->mediator_hash == NULL) {
		/* init hash */
		ctx->mediator_hash = axl_hash_new (axl_hash_string, axl_hash_equal_string);

		/* because the handler was not ready, take oportunity
		   to register built-in events */
		turbulence_mediator_create_plug (ctx, "turbulence", "module-registered",
						 /* do not subscribe */
						 axl_false, NULL, NULL);
		
	} /* end if */
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
	case TURBULENCE_MEDIATOR_ATTR_EVENT_DATA4:
		return (axlPointer)object->event_data4;
	}
	/* requested an unsupported value */
	return NULL;
}

/** 
 * @brief Allows to configure a result data on the provided mediator
 * object. This is only useful for implementers building api call
 * handlers.
 *
 * @param object The object to be configured with the provided result.
 * @param result A pointer to the result to be notified.
 */ 
void        turbulence_mediator_object_set_result (TurbulenceMediatorObject * object,
						   axlPointer                 result)
{
	v_return_if_fail (object);
	object->result = result;
	return;
}

void turbulence_mediator_plug_free (axlPointer _plug)
{
	TurbulenceMediatorPlug * plug = (TurbulenceMediatorPlug * ) _plug;

	axl_free      (plug->entry_domain);
	axl_free      (plug->entry_name);
	axl_list_free (plug->subscribers);
	axl_free      (plug);
	return;
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
 * @param ctx The turbulence context where the plug will be created.
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
 *
 * @return axl_true if the plug was created, otherwise axl_false is returned.
 */
axl_bool turbulence_mediator_create_plug (TurbulenceCtx             * ctx,
					  const char                * entry_name, 
					  const char                * entry_domain,
					  axl_bool                    subscribe,
					  TurbulenceMediatorHandler   handler, 
					  axlPointer                  user_data)
{
	char                         * full_name;
	TurbulenceMediatorPlug       * plug;
	TurbulenceMediatorSubscriber * subscriber;

	v_return_val_if_fail (ctx,          axl_false);
	v_return_val_if_fail (entry_name,   axl_false);
	v_return_val_if_fail (entry_domain, axl_false);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return axl_false;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* get plug */
	plug      = axl_hash_get (ctx->mediator_hash, full_name);
	if (plug == NULL) {
		/* plug not found, create and register */
		plug               = axl_new (TurbulenceMediatorPlug, 1);
		plug->is_api       = axl_false;
		plug->entry_name   = axl_strdup (entry_name);
		plug->entry_domain = axl_strdup (entry_domain);
		plug->subscribers  = axl_list_new (axl_list_always_return_1, axl_free);
		
		/* register */
		axl_hash_insert_full (ctx->mediator_hash, 
				      /* store key and its destroy function */
				      full_name, axl_free,
				      /* store plug and its destroy function */
				      plug, turbulence_mediator_plug_free);
	} else {
		/* free key no longer used */
		axl_free (full_name);
	} /* end if */
	
	/* now check if the caller is requesting to subscribe */
	if (! subscribe) {
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		return axl_true;
	} /* end if */

	/* subscribe: create the holder */
	subscriber            = axl_new (TurbulenceMediatorSubscriber, 1);
	subscriber->handler   = handler;
	subscriber->user_data = user_data;

	/* now subscribe */
	axl_list_append (plug->subscribers, subscriber);

	/* unlock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);
	
	return axl_true;
}

/** 
 * @brief Allows to check the number of plugs registered on the
 * particular context.
 *
 * @param ctx The context where the number of plugs will be checked.
 *
 * @return Returns the number of plugs or -1 if it fails.
 */
int      turbulence_mediator_plug_num     (TurbulenceCtx             * ctx)
{
	int result;
	v_return_val_if_fail (ctx, -1);

	vortex_mutex_lock (&ctx->mediator_hash_mutex);
	result = axl_hash_items (ctx->mediator_hash);
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);

	return result;
}

/** 
 * @brief Checks if a particular plug is already registered.
 *
 * @param ctx The context where the plug existance will be checked.
 *
 * @param entry_name The plug entry name to check.
 * @param entry_domain The plug entry domain to check.
 *
 * @return axl_true if the plug exists, otherwise axl_false is
 * returned.
 */
axl_bool turbulence_mediator_plug_exits   (TurbulenceCtx             * ctx,
					   const char                * entry_name,
					   const char                * entry_domain)
{
	char       * full_name;
	axl_bool     result;

	v_return_val_if_fail (ctx,          axl_false);
	v_return_val_if_fail (entry_name,   axl_false);
	v_return_val_if_fail (entry_domain, axl_false);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return axl_false;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* check existance */
	result = (axl_hash_get (ctx->mediator_hash, full_name) != NULL);
	axl_free (full_name);

	/* (un)-lock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);
	
	return result;
}


/** 
 * @brief Allows to subscribe to the provide entry name under the
 * provided entry domain. The handler and user data will be called in
 * the case the event is fired.
 *
 * @param ctx The context where the subscribe operation will take
 * place.
 *
 * @param entry_name The entry name to subscribe on.
 *
 * @param entry_domain The entry domain to subscribe on.
 *
 * @param handler The handler to be called in the case the event is fired.
 *
 * @param user_data A pointer to user defined data, passed to the
 * handler configured.
 *
 * NOTE: if the event does not exists, the handler will won't receive
 * any notification.
 *
 * @return axl_true if the caller was subscribed, otherwise axl_false
 * is returned.
 */
axl_bool turbulence_mediator_subscribe   (TurbulenceCtx             * ctx,
					  const char                * entry_name,
					  const char                * entry_domain,
					  TurbulenceMediatorHandler   handler,
					  axlPointer                  user_data)
{
	char                         * full_name;
	TurbulenceMediatorPlug       * plug;
	TurbulenceMediatorSubscriber * subscriber;
	
	v_return_val_if_fail (ctx, axl_false);
	v_return_val_if_fail (entry_name, axl_false);
	v_return_val_if_fail (entry_domain, axl_false);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return axl_false;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* get plug */
	plug      = axl_hash_get (ctx->mediator_hash, full_name);

	/* free no longer used entry key */
	axl_free (full_name);

	/* check */
	if (plug == NULL || plug->is_api) {
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		return axl_false;
	} /* end if */
	
	/* subscribe: create the holder */
	subscriber            = axl_new (TurbulenceMediatorSubscriber, 1);
	subscriber->handler   = handler;
	subscriber->user_data = user_data;

	/* now subscribe */
	axl_list_append (plug->subscribers, subscriber);

	/* unlock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);

	return axl_true;
}

/** 
 * @brief Allows to create an event API, an entry definition that is
 * callable by other components, allowing the creation to handle the
 * request and provide a reply. Events created by this function cannot
 * be subscribed.
 *
 * @param ctx The turbulence context where API will be created.
 * @param entry_name The API entry name to be created.
 * @param entry_domain The API entry domain to be created.
 * @param handler The handler to be called when this API is called.
 * @param user_data A user defined pointer to be passed to this handler.
 *
 * @return axl_true If the API was created, otherwise axl_false is
 * returned.
 */
axl_bool     turbulence_mediator_create_api   (TurbulenceCtx             * ctx,
					       const char                * entry_name, 
					       const char                * entry_domain,
					       TurbulenceMediatorHandler   handler, 
					       axlPointer                  user_data)
{
	char                         * full_name;
	TurbulenceMediatorPlug       * plug;

	v_return_val_if_fail (ctx,          axl_false);
	v_return_val_if_fail (entry_name,   axl_false);
	v_return_val_if_fail (entry_domain, axl_false);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return axl_false;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* get plug */
	plug      = axl_hash_get (ctx->mediator_hash, full_name);
	if (plug != NULL) {
		/* API already exists */
		axl_free (full_name);

		/* un-lock */
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		return axl_false;
	} /* end if */
	
	/* plug not found, create and register */
	plug               = axl_new (TurbulenceMediatorPlug, 1);
	plug->is_api       = axl_true;
	plug->entry_name   = axl_strdup (entry_name);
	plug->entry_domain = axl_strdup (entry_domain);
	plug->api_handler  = handler;
	plug->user_data    = user_data;
		
	/* register */
	axl_hash_insert_full (ctx->mediator_hash, 
			      /* store key and its destroy function */
			      full_name, axl_free,
			      /* store plug and its destroy function */
			      plug, turbulence_mediator_plug_free);
	
	/* unlock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);
	
	return axl_true;
}

/** 
 * @brief Allows to remove a particular handler registered on a plug
 * identified by entry_name and entry_domain.
 *
 * @param ctx The turbulence context where the operation will take
 * place.
 *
 * @param entry_name The plug entry name where the provided handler must be removed.
 *
 * @param entry_domain The plug entry domain where the provided handler must be removed.
 *
 * @param handler The handler to be removed. This handler must be the same value that was used to subscribe.
 *
 * @param user_data The user defined data registered. This pointer
 * must be the same value that was used to subscribe.
 */ 
void turbulence_mediator_remove_plug (TurbulenceCtx             * ctx,
				      const char                * entry_name,
				      const char                * entry_domain,
				      TurbulenceMediatorHandler   handler,
				      axlPointer                  user_data)
{
	char                         * full_name;
	TurbulenceMediatorPlug       * plug;
	TurbulenceMediatorSubscriber * subscriber;
	int                            iterator;
	
	
	v_return_if_fail (ctx);
	v_return_if_fail (entry_name);
	v_return_if_fail (entry_domain);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* get plug */
	plug      = axl_hash_get (ctx->mediator_hash, full_name);
	if (plug == NULL) {
		/* free no longer used entry key */
		axl_free (full_name);
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		return;
	} /* end if */
	
	/* subscribe: create the holder */
	iterator = 0;
	while (iterator < axl_list_length (plug->subscribers)) {
		/* get subscriber value */
		subscriber = axl_list_get_nth (plug->subscribers, iterator);

		/* check handler and pointer */
		if (subscriber->handler == handler && subscriber->user_data == user_data) {
			/* found item */
			axl_list_remove_at (plug->subscribers, iterator);

			/* unlock */
			vortex_mutex_unlock (&ctx->mediator_hash_mutex);
			return;
		}

		/* next iterator */
		iterator++;
	} /* end while */

	/* unlock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);

	return;
}

axlPointer turbulence_mediator_common_call (TurbulenceCtx             * ctx,
					    axl_bool                    is_api,
					    const char                * entry_name,
					    const char                * entry_domain,
					    axlPointer                  event_data,
					    axlPointer                  event_data2,
					    axlPointer                  event_data3,
					    axlPointer                  event_data4)
{
	char                         * full_name;
	TurbulenceMediatorPlug       * plug;
	TurbulenceMediatorSubscriber * subscriber;
	int                            iterator;
	TurbulenceMediatorHandler      handler;
	TurbulenceMediatorObject     * object;
	axlPointer                     result = NULL;
	
	
	v_return_val_if_fail (ctx, NULL);
	v_return_val_if_fail (entry_name, NULL);
	v_return_val_if_fail (entry_domain, NULL);

	/* check that the plug is not already created */
	full_name = axl_strdup_printf ("%s::%s", entry_name, entry_domain);
	if (full_name == NULL)
		return NULL;

	/* lock */
	vortex_mutex_lock (&ctx->mediator_hash_mutex);

	/* get plug */
	plug      = axl_hash_get (ctx->mediator_hash, full_name);

	/* free no longer used entry key */
	axl_free (full_name);

	if (plug == NULL || (plug->is_api != is_api)) {
		/* plug not found or it is an api */
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		return NULL;
	} /* end if */

	/* prepare the mediator object */
	object               = axl_new (TurbulenceMediatorObject, 1);
	object->ctx          = ctx;
	object->entry_name   = entry_name;
	object->entry_domain = entry_domain;
	object->event_data   = event_data;
	object->event_data2  = event_data2;
	object->event_data3  = event_data3;
	object->event_data4  = event_data4;

	if (plug->is_api) {
		/* get references to handler and user data */
		handler           = plug->api_handler;
		object->user_data = plug->user_data;

		/* unlock during the operation */
		vortex_mutex_unlock (&ctx->mediator_hash_mutex);
		
		/* do the call operation */
		handler (object);
		
		/* lock during the operation */
		vortex_mutex_lock (&ctx->mediator_hash_mutex);

		/* get result */
		result = object->result;
		
	} else {
		/* subscribe: create the holder */
		iterator = 0;
		while (iterator < axl_list_length (plug->subscribers)) {
			/* get subscriber value */
			subscriber = axl_list_get_nth (plug->subscribers, iterator);
			
			/* check null reference */
			if (subscriber == NULL)
				break;
			
			/* get references to handler and user data */
			handler           = subscriber->handler;
			object->user_data = subscriber->user_data;
			
			/* unlock during the operation */
			vortex_mutex_unlock (&ctx->mediator_hash_mutex);
			
			/* do the call operation */
			handler (object);
			
			/* lock during the operation */
			vortex_mutex_lock (&ctx->mediator_hash_mutex);
			
			/* next iterator */
			iterator++;
		} /* end while */
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&ctx->mediator_hash_mutex);

	/* release object */
	axl_free (object);

	return result;
}

/** 
 * @brief Allows to push an event with the provided data, making an
 * notification on all handlers currently registered.
 *
 * @param ctx The turbulence context where the operation will take
 * place.
 *
 * @param entry_name The entry name where the event will be notified.
 *
 * @param entry_domain The entry domain where the event will be
 * notified.
 *
 * @param event_data The user data to be notified to registered
 * handlers.
 *
 * @param event_data2 The second user data pointer to be notified to
 * registered handlers.

 * @param event_data3 The third user data pointer to be notified to
 * registered handlers.
 *
 * @param event_data4 The fourth user data pointer to be notified to
 * registered handlers.
 */ 
void turbulence_mediator_push_event  (TurbulenceCtx             * ctx,
				      const char                * entry_name,
				      const char                * entry_domain,
				      axlPointer                  event_data,
				      axlPointer                  event_data2,
				      axlPointer                  event_data3,
				      axlPointer                  event_data4)
{
	/* do common call */
	turbulence_mediator_common_call (ctx, axl_false, entry_name, entry_domain, event_data, event_data2, event_data3, event_data4);
	return;
}

/** 
 * @brief Allows to perform a call to an entry API defined.
 * 
 * @param ctx The turbulence context where the API call is defined.
 * @param entry_name The API entry name to be called.
 * @param entry_domain The API entry domain to be called.
 *
 * @param event_data First parameter to be passed to the API
 * call. This is specific for each API call. Check its documentation.
 *
 * @param event_data2 Second parameter to be passed to the API
 * call. This is specific for each API call. Check its documentation.
 *
 * @param event_data3 Third parameter to be passed to the API
 * call. This is specific for each API call. Check its documentation.
 *
 * @param event_data4 Fourth parameter to be passed to the API
 * call. This is specific for each API call. Check its documentation.
 *
 * @return A pointer to the result returned by the API call.
 */
axlPointer     turbulence_mediator_call_api     (TurbulenceCtx             * ctx,
						 const char                * entry_name,
						 const char                * entry_domain,
						 axlPointer                  event_data,
						 axlPointer                  event_data2,
						 axlPointer                  event_data3,
						 axlPointer                  event_data4)
{
	/* do common call signaling it is an api call */
	return turbulence_mediator_common_call (ctx, axl_true, entry_name, entry_domain, event_data, event_data2, event_data3, event_data4);
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
	ctx->mediator_hash = NULL;

	/* clear mutex */
	vortex_mutex_destroy (&ctx->mediator_hash_mutex);
	

	return;
}

/** 
 * @}
 */


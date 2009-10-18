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

#include <turbulence.h>

/* local include */
#include <turbulence-ctx-private.h>

typedef struct _TurbulenceConnMgrState {
	VortexConnection * conn;
	TurbulenceCtx    * ctx;
} TurbulenceConnMgrState;

/** 
 * \defgroup turbulence_conn_mgr Turbulence Connection Manager: a module that controls all connections created under the turbulence execution
 */

/** 
 * \addtogroup turbulence_conn_mgr
 * @{
 */

/** 
 * @internal Handler called once the connection is about to be closed.
 * 
 * @param conn The connection to close.
 */
void turbulence_conn_mgr_on_close (VortexConnection * conn, 
				   axlPointer         user_data)
{
	/* get turbulence context */
	TurbulenceConnMgrState * state = user_data;
	TurbulenceCtx          * ctx   = state->ctx;

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* remove from the hash */
	axl_hash_remove (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (state->conn)));

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return;
}


void turbulence_conn_mgr_unref (axlPointer data)
{
	/* get a reference to the state */
	TurbulenceConnMgrState * state = data;
	TurbulenceCtx          * ctx   = state->ctx;

	/* uninstall on close full handler to avoid race conditions */
	if (! vortex_connection_remove_on_close_full (state->conn, turbulence_conn_mgr_on_close, state)) 
		error ("attention, failed to remove on close handler, this could cause problems..");

	/* unref the connection */
	msg ("Unregistering connection: %d (%p)", vortex_connection_get_id ((VortexConnection*) state->conn), state->conn);
	vortex_connection_unref ((VortexConnection*) state->conn, "turbulence-conn-mgr");

	/* nullify and free */
	state->conn = NULL;
	state->ctx  = NULL;
	axl_free (state);

	return;
}

/** 
 * @internal Function called foreach connection created within the
 * turbulence context.
 * 
 * @param conn The connection created.
 *
 * @param user_data User user defined data.
 */
int turbulence_conn_mgr_notify (VortexCtx               * vortex_ctx,
				VortexConnection        * conn, 
				VortexConnection       ** new_conn,
				VortexConnectionStage     conn_state,
				axlPointer                user_data)
{
	/* get turbulence context */
	TurbulenceConnMgrState * state;
	TurbulenceCtx          * ctx = user_data;

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* create state */
	state       = axl_new (TurbulenceConnMgrState, 1);
	state->conn = conn;
	vortex_connection_ref (conn, "turbulence-conn-mgr");
	state->ctx  = ctx;

	/* store in the hash */
	msg ("Registering connection: %d (%p)", vortex_connection_get_id (conn), conn);
	axl_hash_insert_full (ctx->conn_mgr_hash, 
			      /* key to store */
			      INT_TO_PTR (vortex_connection_get_id (conn)), NULL,
			      /* data to store */
			      state, turbulence_conn_mgr_unref);

	/* configure on close */
	vortex_connection_set_on_close_full (conn, turbulence_conn_mgr_on_close, state);

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* signal no error was found and the rest of handler can be
	 * executed */
	return 1;
}

axl_bool turbulence_conn_mgr_int_foreach (axlPointer key, axlPointer data, axlPointer user_data) 
{
	TurbulenceConnMgrState * state = (TurbulenceConnMgrState *) data;

	/* do not close transport */
	vortex_connection_set_close_socket (state->conn, axl_false);

	/* do not stop foreach process until the last item */
	return axl_false;
}

axlDoc * turbulence_conn_mgr_show_connections (const char * line,
					       axlPointer   user_data, 
					       axl_bool   * status)
{
	TurbulenceCtx          * ctx = (TurbulenceCtx *) user_data;
	axlDoc                 * doc;
	axlHashCursor          * cursor;
	TurbulenceConnMgrState * state;
	axlNode                * parent;
	axlNode                * node;

	/* signal command completed ok */
	(* status) = axl_true;

	/* build document */
	doc = axl_doc_parse_strings (NULL, 
				     "<table>",
				     "  <title>BEEP peers connected</title>",
				     "  <description>The following is a list of peers connected</description>",
				     "  <content></content>",
				     "</table>");
	/* get parent node */
	parent = axl_doc_get (doc, "/table/content");

	/* lock connections */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	while (axl_hash_cursor_has_item (cursor)) {
		/* get connection */
		state = axl_hash_cursor_get_value (cursor);

		/* build connection status information */
		node  = axl_node_parse (NULL, "<row id='%d' host='%s' port='%s' local-addr='%s' local-port='%s' />",
					vortex_connection_get_id (state->conn),
					vortex_connection_get_host (state->conn),
					vortex_connection_get_port (state->conn),
					vortex_connection_get_local_addr (state->conn),
					vortex_connection_get_local_port (state->conn));

		/* set node to result document */
		axl_node_set_child (parent, node);

		/* next cursor */
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* unlock connections */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);
	
	return doc;
}

/** 
 * @internal Function used to catch modules registered. In the case
 * radmin module is found, publish a list of commands to manage
 * connections.
 */
void turbulence_conn_mgr_module_registered (TurbulenceMediatorObject * object)
{
	TurbulenceCtx * ctx  = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_CTX);
	const char    * name = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);

	/* check for radmin module */
	if (axl_cmp (name, "mod_radmin")) {
		/* register commands */
		turbulence_mediator_call_api (ctx, "mod-radmin", "command-install",
					      /* first parameter is the command */
					      "show connections",
					      /* second parameter is the command description */
					      "All to show all connections being handled by turbulence",
					      /* third parameter is the handler */
					      turbulence_conn_mgr_show_connections,
					      /* fourth parameter */
					      ctx);
	} /* end if */

	return;
}

/** 
 * @internal Module init.
 */
void turbulence_conn_mgr_init (TurbulenceCtx * ctx, axl_bool reinit)
{
	VortexCtx  * vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	/* init mutex */
	vortex_mutex_create (&ctx->conn_mgr_mutex);

	/* check for reinit operation */
	if (reinit) {
		axl_hash_foreach (ctx->conn_mgr_hash, turbulence_conn_mgr_int_foreach, ctx);
		axl_hash_free (ctx->conn_mgr_hash);
		ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);
		return;
	}

	/* init connection list hash */
	if (ctx->conn_mgr_hash == NULL) {
		
		ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);

		/* configure notification handlers */
		vortex_connection_set_connection_actions (vortex_ctx,
							  CONNECTION_STAGE_POST_CREATED,
							  turbulence_conn_mgr_notify, ctx);
		
	} /* end if */

	/* register on load module to install radmin connection
	   commands */
	turbulence_mediator_subscribe (ctx, "turbulence", "module-registered", 
				       turbulence_conn_mgr_module_registered, NULL);

	return;
}

typedef struct _TurbulenceBroadCastMsg {
	const void      * message;
	int               message_size;
	const char      * profile;
	TurbulenceCtx   * ctx;
} TurbulenceBroadCastMsg;

int  _turbulence_conn_mgr_broadcast_msg_foreach (axlPointer key, axlPointer data, axlPointer user_data)
{
	VortexChannel          * channel   = data;
	TurbulenceBroadCastMsg * broadcast = user_data;
	TurbulenceCtx          * ctx       = broadcast->ctx;

	/* check the channel profile */
	if (! axl_cmp (vortex_channel_get_profile (channel), broadcast->profile)) 
		return axl_false;

	/* channel found send the message */
	msg ("sending notification on channel running profile: %s", broadcast->profile);
	vortex_channel_send_msg (channel, broadcast->message, broadcast->message_size, NULL);

	/* always return axl_true to make the process to continue */
	return axl_false;
}

/** 
 * @brief General purpose function that allows to broadcast the
 * provided message (and size), over all channel, running the profile
 * provided, on all connections registered. 
 *
 * The function is really useful if it is required to perform a
 * general broadcast to all BEEP peers connected running a particular
 * profile. The function produces a MSG frame with the content
 * provided.
 *
 * The function also allows to configure the connections that aren't
 * broadcasted. To do so, pass the function (filter_conn) that
 * configures which connections receives the notification and which
 * not.
 * 
 * @param ctx Turbulence context where the operation will take place.
 * 
 * @param message The message that is being broadcasted.
 *
 * @param message_size The message size to broadcast.
 *
 * @param profile The profile to search for in all connections
 * registered. If a channel is found running this profile, then a
 * message is sent.
 *
 * @param filter_conn Connection filtering function. If it returns
 * axl_true, the connection is filter.
 *
 * @param filter_data User defined data provided to the filter
 * function.
 * 
 * @return axl_true if the broadcast message was sent to all
 * connections. The function could return axl_false but it has no support
 * to notify which was the connection(s) or channel(s) that failed.
 */
int  turbulence_conn_mgr_broadcast_msg (TurbulenceCtx            * ctx,
					const void               * message,
					int                        message_size,
					const char               * profile,
					TurbulenceConnMgrFilter    filter_conn,
					axlPointer                 filter_data)
{

	/* get turbulence context */
	axlHashCursor          * cursor;
	VortexConnection       * conn;
	int                      conn_id;
	TurbulenceBroadCastMsg * broadcast;
	TurbulenceConnMgrState * state;

	v_return_val_if_fail (message, axl_false);
	v_return_val_if_fail (message_size >= 0, axl_false);
	v_return_val_if_fail (profile, axl_false);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	/* create the broadcast data */
	broadcast               = axl_new (TurbulenceBroadCastMsg, 1);
	broadcast->message      = message;
	broadcast->message_size = message_size;
	broadcast->profile      = profile;
	broadcast->ctx          = ctx;

	while (axl_hash_cursor_has_item (cursor)) {
		
		/* get data */
		conn_id = PTR_TO_INT (axl_hash_cursor_get_key (cursor));
		state   = axl_hash_cursor_get_value (cursor);
		conn    = state->conn;

		msg ("Check for broadcast on connection id=%d", conn_id);

		/* check filter function */
		if (filter_conn != NULL && filter_conn (conn, filter_data)) {

			msg ("Broadcast on connection id=%d filtered", conn_id);

			/* connection filtered */
			axl_hash_cursor_next (cursor);
			continue;
		} /* end if */

		msg ("Doing broadcasting on connection id=%d (%p)", conn_id, conn);

		/* search for channels running the profile provided */
		if (! vortex_connection_foreach_channel (conn, _turbulence_conn_mgr_broadcast_msg_foreach, broadcast))
			error ("failed to broacast message over connection id=%d", vortex_connection_get_id (conn));

		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* free cursor hash and broadcast message */
	axl_hash_cursor_free (cursor);
	axl_free (broadcast);

	return axl_true;
}

void turbulence_conn_mgr_conn_list_free_item (axlPointer _conn)
{
	vortex_connection_unref ((VortexConnection *) _conn, "conn-mgr-list");
	return;
}

/** 
 * @brief Allows to get a list of connections registered on the
 * connection manager, matching the providing role and then filtered
 * by the provided filter string. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param role Connection role to select connections. Use -1 to select
 * all connections registered on the manager, no matter its role. 
 *
 * @param filter Optional filter expresion to resulting connection
 * list. It can be NULL.
 *
 * @return A newly allocated connection list having on each position a
 * reference to a VortexConnection object. The caller must finish the
 * list with axl_list_free to free resources. The function returns NULL if it fails.
 */
axlList *  turbulence_conn_mgr_conn_list   (TurbulenceCtx            * ctx, 
					    VortexPeerRole             role,
					    const char               * filter)
{
	axlList                * result;
	VortexConnection       * conn;
	axlHashCursor          * cursor;
	TurbulenceConnMgrState * state;

	v_return_val_if_fail (ctx, NULL);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);
	result = axl_list_new (axl_list_always_return_1, turbulence_conn_mgr_conn_list_free_item);

	msg ("connections registered: %d..", axl_hash_items (ctx->conn_mgr_hash));

	while (axl_hash_cursor_has_item (cursor)) {
		
		/* get data */
		state   = axl_hash_cursor_get_value (cursor);
		conn    = state->conn;

		/* check connection role to add it to the result
		   list */
		msg ("Checking connection role %d == %d", vortex_connection_get_role (conn), role);
		if ((role == -1) || vortex_connection_get_role (conn) == role) {
			/* update reference and add the connection */
			vortex_connection_ref (conn, "conn-mgr-list");
			axl_list_append (result, conn);
		} /* end if */
		
		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* return list */
	return result;
}

/** 
 * @internal Module cleanup.
 */
void turbulence_conn_mgr_cleanup (TurbulenceCtx * ctx)
{
	
	/* destroy mutex */
	axl_hash_free (ctx->conn_mgr_hash);
	ctx->conn_mgr_hash = NULL;

	vortex_mutex_destroy (&ctx->conn_mgr_mutex);

	return;
}

/** 
 * @}
 */

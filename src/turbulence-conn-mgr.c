/*  Turbulence:  BEEP application server
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

#include <turbulence.h>

/* local include */
#include <turbulence-ctx-private.h>

/**
 * \defgroup turbulence_conn_mgr Turbulence Connection Manager: a module that controls all connections created under the turbulence execution
 */

/**
 * \addtogroup turbulence_conn_mgr
 * @{
 */

void turbulence_conn_mgr_unref (axlPointer data)
{
	/* unref the connection */
	msg ("Unregistering connection: %d", vortex_connection_get_id ((VortexConnection*) data));
	vortex_connection_unref ((VortexConnection*) data, "turbulence-conn-mgr");

	return;
}


/** 
 * @internal Handler called once the connection is about to be closed.
 * 
 * @param conn The connection to close.
 */
void turbulence_conn_mgr_on_close (VortexConnection * conn)
{
	/* get turbulence context */
	TurbulenceCtx    * ctx = turbulence_ctx_get ();

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* remove from the hash */
	axl_hash_remove (ctx->conn_mgr_hash, INT_TO_PTR (vortex_connection_get_id (conn)));

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

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
void turbulence_conn_mgr_notify (VortexConnection * conn, axlPointer user_data)
{
	/* get turbulence context */
	TurbulenceCtx    * ctx = turbulence_ctx_get ();

	/* new connection created: configure it */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);

	/* store in the hash */
	vortex_connection_ref (conn, "turbulence-conn-mgr");
	msg ("Registering connection: %d", vortex_connection_get_id (conn));
	axl_hash_insert_full (ctx->conn_mgr_hash, 
			      /* key to store */
			      INT_TO_PTR (vortex_connection_get_id (conn)), NULL,
			      /* data to store */
			      conn, turbulence_conn_mgr_unref);

	/* configure on close */
	vortex_connection_set_on_close (conn, turbulence_conn_mgr_on_close);

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	return;
}

/** 
 * @internal Module init.
 */
void turbulence_conn_mgr_init (TurbulenceCtx * ctx)
{
	/* init mutex */
	vortex_mutex_create (&ctx->conn_mgr_mutex);

	/* init connection list hash */
	ctx->conn_mgr_hash = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* configure notification handlers */
	vortex_connection_notify_new_connections (turbulence_conn_mgr_notify, NULL);

	return;
}

typedef struct _TurbulenceBroadCastMsg {
	const void * message;
	int          message_size;
	const char * profile;
} TurbulenceBroadCastMsg;

bool _turbulence_conn_mgr_broadcast_msg_foreach (axlPointer key, axlPointer data, axlPointer user_data)
{
	VortexChannel          * channel   = data;
	TurbulenceBroadCastMsg * broadcast = user_data;

	/* check the channel profile */
	if (! axl_cmp (vortex_channel_get_profile (channel), broadcast->profile)) 
		return false;

	/* channel found send the message */
	msg ("sending notification on channel running profile: %s", broadcast->profile);
	vortex_channel_send_msg (channel, broadcast->message, broadcast->message_size, NULL);

	/* always return true to make the process to continue */
	return false;
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
 * @param message The message that is being broadcasted.
 *
 * @param message_size The message size to broadcast.
 *
 * @param profile The profile to search for in all connections
 * registered. If a channel is found running this profile, then a
 * message is sent.
 *
 * @param filter_conn Connection filtering function. If it returns
 * true, the connection is filter.
 *
 * @param filter_data User defined data provided to the filter
 * function.
 * 
 * @return true if the broadcast message was sent to all
 * connections. The function could return false but it has no support
 * to notify which was the connection(s) or channel(s) that failed.
 */
bool turbulence_conn_mgr_broadcast_msg (const void * message,
					int          message_size,
					const char * profile,
					TurbulenceConnMgrFilter filter_conn,
					axlPointer              filter_data)
{

	/* get turbulence context */
	TurbulenceCtx          * ctx = turbulence_ctx_get ();
	axlHashCursor          * cursor;
	VortexConnection       * conn;
	int                      conn_id;
	TurbulenceBroadCastMsg * broadcast;

	v_return_val_if_fail (message, false);
	v_return_val_if_fail (message_size >= 0, false);
	v_return_val_if_fail (profile, false);

	/* lock and send */
	vortex_mutex_lock (&ctx->conn_mgr_mutex);
	
	/* create the cursor */
	cursor = axl_hash_cursor_new (ctx->conn_mgr_hash);

	/* create the broadcast data */
	broadcast               = axl_new (TurbulenceBroadCastMsg, 1);
	broadcast->message      = message;
	broadcast->message_size = message_size;
	broadcast->profile      = profile;

	while (axl_hash_cursor_has_item (cursor)) {
		
		/* get data */
		conn_id = PTR_TO_INT (axl_hash_cursor_get_key (cursor));
		conn    = axl_hash_cursor_get_value (cursor);

		msg ("Check for broadcast on connection id=%d", conn_id);

		/* check filter function */
		if (filter_conn != NULL && filter_conn (conn, filter_data)) {

			msg ("Broadcast on connection id=%d filtered", conn_id);

			/* connection filtered */
			axl_hash_cursor_next (cursor);
			continue;
		} /* end if */

		/* search for channels running the profile provided */
		vortex_connection_foreach_channel (conn, _turbulence_conn_mgr_broadcast_msg_foreach, broadcast);

		/* next cursor */
		axl_hash_cursor_next (cursor);
	}

	/* unlock */
	vortex_mutex_unlock (&ctx->conn_mgr_mutex);

	/* free cursor hash and broadcast message */
	axl_hash_cursor_free (cursor);
	axl_free (broadcast);

	return true;
}

/** 
 * @internal Module cleanup.
 */
void turbulence_conn_mgr_cleanup (TurbulenceCtx * ctx)
{
	
	/* destroy mutex */
	vortex_mutex_destroy (&ctx->conn_mgr_mutex);
	axl_hash_free (ctx->conn_mgr_hash);
	ctx->conn_mgr_hash = NULL;

	return;
}

/**
 * @}
 */

/*  Turbulence BEEP application server
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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
#include <mod_radmin.h>
#include <time.h>

/* include private turbulence headers */
#include <turbulence-ctx-private.h>

/* include sasl support */
#include <vortex_sasl.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* reference to the set of commands installed */
axlList      * commands;
VortexMutex    commands_mutex;

typedef axlDoc * (*ModRadminCommandHandler) (const char * arguments, axlPointer user_data, axl_bool * result);

typedef struct _ModRadminCommandItem {
	char                     * command;
	int                        length;
	char                     * description;
	ModRadminCommandHandler    handler;
	axlPointer                 user_data;
} ModRadminCommandItem;

void mod_radmin_command_item_free (axlPointer data) {
	ModRadminCommandItem * item = (ModRadminCommandItem *) data;

	/* free command, description and the item itself */
	axl_free (item->command);
	axl_free (item->description);
	axl_free (item);
	return;
}

/** 
 * @internal Function used by mod-radmin to find handler associated to
 * the command provided.
 */
ModRadminCommandItem * mod_radmin_find_handler (const char * command)
{
	ModRadminCommandItem    * cmd  = NULL;
	ModRadminCommandItem    * last = NULL;
	int                       max_length;
	int                       iterator;

	/* lock mutex */
	vortex_mutex_lock (&commands_mutex);
	
	/* try to find a command that could handle the command */
	iterator   = 0;
	max_length = 0;
	while (iterator < axl_list_length (commands)) {
		
		/* get command */
		cmd = axl_list_get_nth (commands, iterator);

		/* check if the command matches */
		if (axl_memcmp (cmd->command, command, cmd->length)) {
			/* found command that matches */
			if (cmd->length > max_length) {
				/* record max length */
				max_length = cmd->length;
				/* record last matched */
				last = cmd;
			} /* end if */
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	if (last != NULL)
		msg ("selected command handler for: %s", last->command);

	/* unlock mutex */
	vortex_mutex_unlock (&commands_mutex);
	return last;
}

/** 
 * @internal Function used to build error messages easily
 */
axlDoc * mod_radmin_error_msg (int code, const char * message)
{
	axlDoc   * result;
	axlError * err = NULL;
	char     * _message = axl_strdup_printf ("<error code=\"%d\"><![CDATA[%s]]></error>",
						 code, message);
	
	result = axl_doc_parse (_message, strlen (_message), &err);
	axl_free (_message);
	if (result == NULL) {
		error ("Failed to build error message, failure was: %s", axl_error_get (err));
		axl_error_free (err);
		return NULL;
	}

	return result;
}

/** 
 * @internal Function used to build ok messages easily
 */
axlDoc * mod_radmin_ok_msg (int code, const char * title, const char * message)
{
	axlDoc   * result;
	axlError * err = NULL;
	char     * _message = axl_strdup_printf ("<table><title>%s</title><column-description><column name='code' description='Status code from reload operation' /><column name='message' description='Status textual message' /></column-description><content><row><d>%d</d><d>%s</d></row></content></table>",
						 title, code, message);
	
	result = axl_doc_parse (_message, strlen (_message), &err);
	axl_free (_message);
	if (result == NULL) {
		error ("Failed to build error message, failure was: %s", axl_error_get (err));
		axl_error_free (err);
		return NULL;
	}

	return result;
}


/** 
 * @internal Function used to handle command reply (taking result and
 * sending it to the caller), releasing all resources and checking all
 * errors.
 */
void mod_radmin_handle_command_reply (axl_bool           status, 
				      axlDoc           * result, 
				      VortexConnection * conn, 
				      VortexChannel    * channel, 
				      VortexFrame      * frame)
{
	char  * str_result;
	int     str_size;

	if (! status) {
		/* dump document */
		axl_doc_dump (result, &str_result, &str_size);

		vortex_channel_send_err (channel,
					 str_result, 
					 str_size,
					 vortex_frame_get_msgno (frame));

		/* free document returned by command handler */
	        axl_free (str_result);
		axl_doc_free (result);
		return;
	} 

	/* FIXME: implement here format conversion */

	/* build string representation */
	if (! axl_doc_dump (result, &str_result, &str_size)) {
		vortex_channel_send_errv (channel, 
					  vortex_frame_get_msgno (frame),
					  "<error code=\"8\">Command handler returned an xml document that failed to be dumped.</error>");
		/* free document returned by command handler */
		axl_doc_free (result);
		return;
	}
	
	/* return content */
	vortex_channel_send_rpy (channel,
				 str_result, str_size,
				 vortex_frame_get_msgno (frame));

	/* free result returned by command handler */
	axl_free (str_result);
	axl_doc_free (result);

	return;
}

/** 
 * @internal Function that handles an incoming request checking if
 * there is a command handler registered for the command operation
 * received.
 *
 * The function finds the handler associated to the command, run it
 * and then call to mod_radmin_handle_command_reply.
 */
void mod_radmin_handle_command (VortexConnection * conn, 
				VortexChannel    * channel, 
				axlDoc           * doc, 
				VortexFrame      * frame)
{
	/* according to the operation, do */
	axlNode              * node    = axl_doc_get_root (doc);
	const char           * command = ATTR_VALUE (node, "operation");
	ModRadminCommandItem * cmd;
	axlDoc               * result;
	axl_bool               status;
	
	if (command == NULL) {
		/* no command found, protocol error, close */
		error ("No command found on incoming request, protocol error, shutdown connection");
		vortex_connection_shutdown (conn);
		return;
	} /* end if */

	/* prepare command removing all unneeded elements */
	axl_stream_trim ((char *)command);
	axl_stream_to_lower ((char *)command);
	
	/* get command */
	/* lock mutex */
	msg ("checking handler for command: %s..", command);
	cmd = mod_radmin_find_handler (command);

	if (cmd == NULL) {
		vortex_channel_send_errv (channel,
					  vortex_frame_get_msgno (frame),
					  "<error code='5'>No command handler was found associated to %s</error>",
					  command);
		return;
	} /* end if */

	/* ok, now call to handle command and return content */
	status = axl_false;
	result = cmd->handler (command, cmd->user_data, &status);

	/* call to handle reply */
	mod_radmin_handle_command_reply (status, result, conn, channel, frame);
	return;
}

axlDoc * mod_radmin_command_reload (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc   * doc;
	axlError * err = NULL;

	/* signal command returned proper status */
	(*status) = axl_true;

	/* call to reload */
	turbulence_reload_config (ctx, 0);

	/* result document */
	doc = axl_doc_parse_strings (&err, 
				     "<table>",
				     " <title>Reload status</title>",
				     " <column-description>",
				     "   <column name='code' description='Status code from reload operation' />",
				     "   <column name='message' description='Status textual message' />",
				     " </column-description>",
				     " <content><row><d>0</d><d>reload ok</d></row></content>",
				     "</table>", NULL);

	return doc;
}

axlDoc * mod_radmin_command_status (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc        * doc;
	axlError      * err = NULL;
	axlNode       * node;
	int             running_threads;
	int             waiting_threads;
	int             pending_tasks;
	struct timeval  tv;

	/* signal command returned proper status */
	(*status) = axl_true;

	/* get pool stats */
	vortex_thread_pool_stats (TBC_VORTEX_CTX (ctx), &running_threads, &waiting_threads, &pending_tasks);

	/* get time of day */
	gettimeofday (&tv, NULL);
 
	/* result document */
	doc = axl_doc_parse_strings (&err, 
				     "<table>",
				     " <title>Status</title>",
				     " <column-description>",
				     "   <column name='Indicator' description='Status code' />",
				     "   <column name='Value' description='Status message' />",
				     " </column-description>",
				     " <content>", 
				     " </content>",
				     "</table>", NULL);
	node = axl_doc_get (doc, "/table/content");
	if (node) {
		/* add info */
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Childs</d><d>%d</d></row>", turbulence_process_child_count (ctx)));
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Connections</d><d>%d</d></row>", turbulence_conn_mgr_count (ctx)));
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Thread pool size</d><d>%d</d></row>", running_threads));
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Waiting threads</d><d>%d</d></row>", waiting_threads));
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Pending tasks</d><d>%d</d></row>", pending_tasks));
		axl_node_set_child (node, axl_node_parse (NULL, "<row><d>Secs. running</d><d>%ld</d></row>", (tv.tv_sec - ctx->running_stamp)));
	} /* end if */

	return doc;
}

const char * mod_radmin_role_to_string (VortexPeerRole role)
{
	switch (role) {
	case VortexRoleInitiator:
		return "initiator";
	case VortexRoleListener:
		return "listener";
	case VortexRoleMasterListener:
		return "master";
	case VortexRoleUnknown:
	default:
		break;
	}
	return "unknown";
}

typedef void (*ModRadminChildCommandHandler) (TurbulenceCtx * ctx, const char * content, axl_bool status, axlPointer user_data);


/** 
 * @internal Function that allows to send a command to a particular
 * child using internal conn mgr connection.
 *
 * @param ctx The context where the module is working.
 *
 * @param child The child process that will receive the command.
 *
 * @param queue Reference to the queue used to receive the reply or
 * NULL to make the function to create and release a queue internally.
 *
 * @param command The command to send.
 *
 * @return The function returns the frame reply from child or NULL if
 * it fails.
 */
VortexFrame * mod_radmin_run_command_on_child (TurbulenceCtx * ctx, TurbulenceChild * child, VortexAsyncQueue * queue, const char * command)
{
	VortexChannelPool * pool;
	VortexConnection  * conn;
	VortexChannel     * channel;
	VortexFrame       * reply;
	axl_bool            release_queue = (queue == NULL);
	
	/* create a queue if not defined */
	if (queue == NULL)
		queue = vortex_async_queue_new ();

	/* acquire a reference to the child connection and use local
	   reference to avoid having child closed in the middle (kill
	   child) */
	conn  = child->conn_mgr;
	if (! vortex_connection_ref (conn, "begin mod-radmin run child cmd")) {
		error ("Failed to create channel pool to send commands to childs..");
		if (release_queue)
			vortex_async_queue_unref (queue);
		return NULL;
	}

	/* check if we have a pool created */
	pool = vortex_connection_get_channel_pool (conn, 1);
	if (pool == NULL) {
		/* pool not created, create one */
		pool = vortex_channel_pool_new (conn, 
						RADMIN_URI_INTERNAL,
						/* one channel initially */
						1, 
						/* on close handler */
						NULL, NULL,
						/* on frame received handler */
						NULL, NULL,
						/* on channel created */
						NULL, NULL);
		if (pool == NULL) {
			error ("Failed to create channel pool to send commands to childs..");
			if (release_queue)
				vortex_async_queue_unref (queue);

			/* release connection */
			vortex_connection_unref (conn, "end mod-radmin run child cmd");
			return NULL;
		} /* end if */
	} /* end if */

	/* get a channel from the pool */
	channel = vortex_channel_pool_get_next_ready (pool, axl_true);
	if (channel == NULL) {
		error ("Unable to get a channel ready from the pool to send command to child..");
		if (release_queue)
			vortex_async_queue_unref (queue);
		/* release connection */
		vortex_connection_unref (conn, "end mod-radmin run child cmd");
		return NULL;
	} /* end if */

	/* configure frame received */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	/* and send command */
	if (! vortex_channel_send_msg (channel, command, strlen (command), NULL)) {
		error ("Unable to send command to child process..");
		if (release_queue)
			vortex_async_queue_unref (queue);

		/* release connection */
		vortex_connection_unref (conn, "end mod-radmin run child cmd");
		return NULL;
	} /* end if */

	/* now wait for a reply */
	reply = vortex_channel_get_reply (channel, queue);

	/* release the channel */
	vortex_channel_pool_release_channel (pool, channel);

	if (release_queue)
		vortex_async_queue_unref (queue);

	/* release connection */
	vortex_connection_unref (conn, "end mod-radmin run child cmd");
	return reply;
}


void mod_radmin_run_command_on_childs (TurbulenceCtx * ctx, const char * command,
				       ModRadminChildCommandHandler handler, axlPointer user_data)
{
	axlList          * childs;
	axlListCursor    * cursor;
	TurbulenceChild  * child;
	VortexAsyncQueue * queue;
	VortexFrame      * reply;

	/* get list of childs */
	childs = turbulence_process_child_list (ctx);
	if (axl_list_length (childs) == 0) {
		/* no childs, finish */
		axl_list_free (childs);
		return;
	} /* end if */

	/* init queue */
	queue = vortex_async_queue_new ();

	/* now for each child, call the handler */
	cursor = axl_list_cursor_new (childs);
	while (axl_list_cursor_has_item (cursor)) {

		/* get child */
		child = axl_list_cursor_get (cursor);

		/* run the command on the child */
		msg ("Running command %s on child %d", command, child->pid);
		reply = mod_radmin_run_command_on_child (ctx, child, queue, command);

		if (reply) {
			/* notify on the handler */
			handler (ctx, 
				 /* content */
				 (const char *) vortex_frame_get_payload (reply), 
				 /* status */
				 vortex_frame_get_type (reply) == VORTEX_FRAME_TYPE_RPY,
				 user_data);
		} /* end if */

		/* release frame */
		vortex_frame_unref (reply);

		/* next item */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* release queue */
	msg ("Command %s on childs finished, release queue and child list", command);
	vortex_async_queue_unref (queue);
	axl_list_free (childs);
	axl_list_cursor_free (cursor);

	return;
}

void mod_radmin_child_show_connections_handler (TurbulenceCtx * ctx, const char * content, axl_bool status, axlPointer user_data)
{
	axlDoc   * child_doc;
	axlDoc   * doc = user_data;
	axlError * err = NULL;
	axlNode  * node;
	axlNode  * child_node;
	axlNode  * aux;

	/* parse child doc */
	child_doc = axl_doc_parse (content, -1, &err);
	if (child_doc == NULL) {
		error ("Failed to parse content from child, error was: %s", axl_error_get (err));
		axl_error_free (err);
		return;
	} /* end if */

	/* get reference to first connection row */
	child_node = axl_doc_get (child_doc, "/table/content/row");
	node       = axl_doc_get (doc, "/table/content");
	while (child_node) {
		/* configure current pointer and get next */
		aux = axl_node_get_next_called (child_node, "row");

		/* move child_node from document */
		child_node = axl_node_copy (child_node, axl_true, axl_true);

		/* set as child of the parent document */
		axl_node_set_child (node, child_node);
		
		/* next node */
		child_node = aux;
	}

	/* free document parsed */
	axl_doc_free (child_doc);

	return;
}
				       

axlDoc * mod_radmin_command_show_connections (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc           * doc;
	axlError         * err       = NULL;
	axlList          * conn_list = NULL;
	axlNode          * node;
	axlNode          * content;
	axlListCursor    * cursor;
	VortexConnection * conn;
	const char       * role;
	long               bytes_sent;
	long               bytes_recv;
	long               activity_stamp;
	char             * time_str;

	/* get the list of connections */
	conn_list = turbulence_conn_mgr_conn_list (ctx, -1, NULL);
	
	if (conn_list == NULL) {
		(* status) = axl_false;
		return NULL;
	} /* end if */

	/* result document */
	doc = axl_doc_parse_strings (&err, 
				     "<table>",
				     " <title>Connection list</title>",
				     " <column-description>",
				     "   <column name='proc-id' description='Process ID' />",
				     "   <column name='conn-id' description='Conn ID' />",
				     "   <column name='role' description='Connection role' />",
				     "   <column name='source' description='Source' />",
				     "   <column name='dest' description='Source' />",
				     "   <column name='channels opened' description='Channels opened' />",
				     "   <column name='ppath' description='Profile path' />",
				     "   <column name='auth id' description='Auth id on this connection or - if not selected' />",
				     "   <column name='last activity' description='When was last activity detected on this connection' />",
				     "   <column name='bytes recv' description='Bytes received on this connection' />",
				     "   <column name='bytes sent' description='Bytes sent on this connection' />",
				     " </column-description>",
				     " <content></content>",
				     "</table>", NULL);

	if (doc == NULL) {
		/* free list */
		axl_list_free (conn_list);

		(* status) = axl_false;
		return NULL;
	} /* end if */

	/* get the content node and populate it */
	content = axl_doc_get (doc, "/table/content");
	cursor  = axl_list_cursor_new (conn_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* get item */
		conn = axl_list_cursor_get (cursor);

		role = mod_radmin_role_to_string (vortex_connection_get_role (conn));

		/* get connection activity status */
		vortex_connection_get_receive_stamp (conn, &bytes_recv, &bytes_sent, &activity_stamp);
		if (activity_stamp == 0)
			time_str = axl_strdup ("-");
		else
			time_str = axl_strdup (ctime (&activity_stamp));
		axl_stream_trim (time_str);

		/* build node */
		node = axl_node_parse (NULL, "<row><d>%d</d><d>%d</d><d>%s</d><d>%s:%s</d><d>%s:%s</d><d>%d</d><d>%s</d><d>%s</d><d>%s</d><d>%ld</d><d>%ld</d></row>", 
				       vortex_getpid (),
				       vortex_connection_get_id (conn),
				       role,
				       vortex_connection_get_host (conn), vortex_connection_get_port (conn),
				       vortex_connection_get_local_addr (conn), vortex_connection_get_local_port (conn),
				       vortex_connection_channels_count (conn),
				       /* ppath */
				       turbulence_ppath_selected (conn) ? turbulence_ppath_get_name (turbulence_ppath_selected (conn)) : "-",
				       /* user (if any) */
				       AUTH_ID_FROM_CONN (conn) ? AUTH_ID_FROM_CONN (conn) : "-",
				       /* last activity */
				       time_str,
				       /* bytes received */
				       bytes_recv,
				       /* bytes_sent */
				       bytes_sent);

		/* free time string */
		axl_free (time_str);
				       
		/* add node to the content */
		axl_node_set_child (content, node);
		
		/* next item */
		axl_list_cursor_next (cursor);
	}

	/* free list and cursor */
	axl_list_cursor_free (cursor);
	axl_list_free (conn_list);

	/* now get connections on childs */
	if (! turbulence_ctx_is_child (ctx)) {
		/* get connectiosn from childs */
		msg ("Getting connections from childs..");
		mod_radmin_run_command_on_childs (ctx, "show connections", 
						  mod_radmin_child_show_connections_handler, doc);
	} /* end if */
	

	/* signal command returned proper status */
	(*status) = axl_true;

	return doc;
}

axl_bool mod_radmin_command_show_channels_foreach (axlPointer key, axlPointer data, axlPointer user_data)
{
	axlNode          * content = user_data;
	axlNode          * node;
	VortexChannel    * channel = data;
	VortexConnection * conn = vortex_channel_get_connection (channel);

	/* build node */
	node = axl_node_parse (NULL, "<row><d>%d</d><d>%d</d><d>%d</d><d>%s</d><d>%s</d><d>%s</d></row>", 
			       /* proc-id */
			       vortex_getpid (),
			       /* conn-id */
			       vortex_connection_get_id (conn),
			       /* channel number */
			       PTR_TO_INT (key),
			       /* channel profile */
			       vortex_channel_get_profile (channel),
			       /* recv-ready */
			       vortex_channel_get_next_reply_no (channel) == -1 ? "ready" : "busy",
			       /* send-ready */
			       vortex_channel_is_ready (channel) ? "ready" : "busy");

	/* add node to the content */
	axl_node_set_child (content, node);
	
	return axl_false; /* do not stop foreach process */
}

axlDoc * mod_radmin_command_show_channels (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc           * doc;
	axlError         * err       = NULL;
	axlList          * conn_list = NULL;
	axlNode          * content;
	axlListCursor    * cursor;
	VortexConnection * conn;

	/* get the list of connections */
	conn_list = turbulence_conn_mgr_conn_list (ctx, -1, NULL);
	
	if (conn_list == NULL) {
		(* status) = axl_false;
		return NULL;
	} /* end if */

	/* result document */
	doc = axl_doc_parse_strings (&err, 
				     "<table>",
				     " <title>Connection list</title>",
				     " <column-description>",
				     "   <column name='proc-id' description='Process ID' />",
				     "   <column name='conn-id' description='Conn ID' />",
				     "   <column name='chann num' description='Channel number' />",
				     "   <column name='profile' description='Source' />",
				     "   <column name='recv-ready' description='Ready to receive' />",
				     "   <column name='send-ready' description='Ready to send' />",
				     " </column-description>",
				     " <content></content>",
				     "</table>", NULL);

	if (doc == NULL) {
		/* free list */
		axl_list_free (conn_list);

		(* status) = axl_false;
		return NULL;
	} /* end if */

	/* get the content node and populate it */
	content = axl_doc_get (doc, "/table/content");
	cursor  = axl_list_cursor_new (conn_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* get item */
		conn = axl_list_cursor_get (cursor);

		/* skip master listeners */
		if (vortex_connection_get_role (conn) == VortexRoleMasterListener) {
			axl_list_cursor_next (cursor);
			continue;
		}

		/* add separator to see channels from same connection */
		axl_node_set_child (content, axl_node_parse (NULL, "<row><d></d><d></d><d></d><d>---- conn-id: %d from: %s:%s ----</d><d></d><d></d></row>", 
							     vortex_connection_get_id (conn), vortex_connection_get_host (conn), vortex_connection_get_port (conn)));

		/* now, for each connection, show each channel */
		vortex_connection_foreach_channel (conn, mod_radmin_command_show_channels_foreach, content);

		/* next item */
		axl_list_cursor_next (cursor);
	}

	/* free list and cursor */
	axl_list_cursor_free (cursor);
	axl_list_free (conn_list);

	/* now get connections on childs */
	if (! turbulence_ctx_is_child (ctx)) {
		/* now get channels from childs */
		mod_radmin_run_command_on_childs (ctx, "show channels", 
						  mod_radmin_child_show_connections_handler, doc);
	} /* end if */

	/* signal command returned proper status */
	(*status) = axl_true;

	return doc;
}


axl_bool mod_radmin_add_childs (axlPointer item, axlPointer user_data) 
{
	TurbulenceChild * child   = item;
	axlNode         * node;
	axlNode         * content = user_data;

	msg ("Adding child..");

	/* add child process */
	node = axl_node_parse (NULL, "<row><d>%d</d><d>%s</d><d>%d</d><d>%s</d></row>", 
			       child->pid,
			       "child",
			       -1,
			       turbulence_ppath_get_name (child->ppath));
	axl_node_set_child (content, node);

	return axl_false; /* keep foreach loop */
}

axlDoc * mod_radmin_command_show_childs (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc           * doc;
	axlError         * err       = NULL;
	axlNode          * node;
	axlNode          * content;
	int                connection_number;
	axlList          * childs;

	/* result document */
	doc = axl_doc_parse_strings (&err, 
				     "<table>",
				     " <title>Process list</title>",
				     " <column-description>",
				     "   <column name='proc-id' description='Process ID' />",
				     "   <column name='proc-type' description='Process type' />",
				     "   <column name='conn-num' description='Connections handled by the process' />",
				     "   <column name='profile-path' description='Profile path selected for the process' />",
				     " </column-description>",
				     " <content></content>",
				     "</table>", NULL);

	if (doc == NULL) {
		(* status) = axl_false;
		return NULL;
	} /* end if */

	/* get the content node and populate it */
	content = axl_doc_get (doc, "/table/content");

	/* get number of connections at the parent */
	childs = turbulence_process_child_list (ctx);
	if (childs == NULL) {
		(* status) = axl_false;
		axl_doc_free (doc);
		return NULL;
	}

	/* get number of childs */
	connection_number = axl_list_length (childs);

	/* add master process */
	node = axl_node_parse (NULL, "<row><d>%d</d><d>%s</d><d>%d</d><d>%s</d></row>",
			       vortex_getpid (),
			       "master",
			       connection_number,
			       "");
	axl_node_set_child (content, node);

	/* iterate all childs declared */
	msg ("Number of childs currently created: %d", connection_number);
	axl_list_foreach (childs, mod_radmin_add_childs, content);

	/* free child list */
	axl_list_free (childs);

	/* signal command returned proper status */
	(*status) = axl_true;

	return doc;
}

axlDoc * mod_radmin_command_kill_child (const char * line, axlPointer user_data, axl_bool * status)
{
	char           ** items;
	int               child_pid;
	VortexFrame     * frame;
	TurbulenceChild * child;
	axlDoc          * result;

	msg ("Splitting command line received: '%s'", line);
	items = axl_split (line, 1, " ");
	
	if (items[2] == NULL)
		return mod_radmin_error_msg (550, "Not provided a child pid");

	child_pid = atoi(items[2]);
	axl_freev (items);

	/* release items */
	msg ("Attempting to kill child: %d", child_pid);

	/* ensure child exists in our list to avoid killing other
	 * process */
	if (! turbulence_process_child_exists (ctx, child_pid)) {
		/* child do not exists */
		return mod_radmin_error_msg (550, "Provided a child process pid not found in master process. Operation cancelled");
	} /* end if */

	/* run command on child */
	child = turbulence_process_child_by_id (ctx, child_pid);
	if (child) {
		/* send kill child command */
		frame = mod_radmin_run_command_on_child (ctx, child, NULL, "kill child");

		/* check for errors */
		if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ERR) {
			/* found error, reply to caller */
			result = mod_radmin_error_msg (550, (const char *) vortex_frame_get_payload (frame));
			vortex_frame_unref (frame);

			return result;
		} /* end if */

		/* unref frame */
		vortex_frame_unref (frame);

		/* reached this point, child kill command was sent */
	} else {
		error ("Child %d not found, unable to kill child", child_pid);
		(*status) = axl_false;
		return mod_radmin_error_msg (550, "Child not found, unable to kill child. Operation cancelled");
	} /* end if */


	/* notify child killed */
	(*status) = axl_true;
	return mod_radmin_ok_msg (200, "Child killed", "Killing signal sent to child");
}

axlDoc * mod_radmin_command_kill_connection (const char * line, axlPointer user_data, axl_bool * status)
{
	char             ** items;
	char             ** aux;
	int                 child_pid;
	int                 conn_id;
	VortexFrame       * frame;
	TurbulenceChild   * child;
	axlDoc            * result;
	VortexConnection  * conn;
	char              * cmd;

	msg ("Splitting command line received: '%s'", line);

	/* check the command have a : */
	if (strstr (line, ":") == NULL)
		return mod_radmin_error_msg (550, "Not provided a pid and connection id to kill, format: pid:conn-id");

	items = axl_split (line, 1, " ");
	
	if (items[2] == NULL)
		return mod_radmin_error_msg (550, "Not provided a pid and connection id to kill, format: pid:conn-id");

	/* ok, now split item to get pid and conn-id */
	aux = axl_split (items[2], 1, ":");
	axl_freev (items);

	child_pid = atoi(aux[0]);
	conn_id  = atoi(aux[1]); 
	axl_freev (aux);

	/* release items */
	msg ("Attempting to kill connection %d on child %d", conn_id, child_pid);

	/* check if we have to kill a connection on parent process */
	if (vortex_getpid () == child_pid) {
		/* ok, it seems we have to close a connection on parent */
		conn = turbulence_conn_mgr_find_by_id (ctx, conn_id);
		if (conn) {
			/* connection found, shutdown */
			vortex_connection_shutdown (conn);

			/* report ok */
			(*status) = axl_true;
			return mod_radmin_ok_msg (200, "Connection killed", "Connection close initiated");
		}

		/* report error */
		return mod_radmin_error_msg (550,  "Unable to find connection on master process");

	} else {
		
		/* ensure child exists in our list */
		if (! turbulence_process_child_exists (ctx, child_pid)) {
			/* child do not exists */
			return mod_radmin_error_msg (550, "Provided a child process pid not found in master process. Operation cancelled");
		} /* end if */

		/* run command on child */
		child = turbulence_process_child_by_id (ctx, child_pid);
		if (child) {
			/* send kill connection command */
			cmd   = axl_strdup_printf ("kill conn %d", conn_id);
			frame = mod_radmin_run_command_on_child (ctx, child, NULL, cmd);
			axl_free (cmd);

			/* unref child */
			turbulence_child_unref (child);

			/* check for errors */
			if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ERR) {
				/* found error, reply to caller */
				result = mod_radmin_error_msg (550, (const char *) vortex_frame_get_payload (frame));
				vortex_frame_unref (frame);
				
				return result;
			} /* end if */

			/* unref frame */
			vortex_frame_unref (frame);
			
			/* reached this point, connection kill command was sent */
		} /* end if */
	} /* end if */

	/* notify connection killed */
	(*status) = axl_true;
	return mod_radmin_ok_msg (200, "Connection killed", "Connection close initiated on child");
}

axlDoc * mod_ramdin_command_commands_available (const char * line, axlPointer user_data, axl_bool * status)
{
	int                    iterator   = 0;
	ModRadminCommandItem * item;
	axlDoc               * result;
	axlNode              * node;
	axlNode              * nodeAux;
	axlError             * err = NULL;

	/* signal command returned proper status */
	(*status) = axl_true;

	/* lock mutex */
	vortex_mutex_lock (&commands_mutex);

	/* result document */
	result = axl_doc_parse_strings (&err, 
					"<table>",
					" <title>List of commands available</title>",
					" <description>The following is the list of commands that can be used.</description>",
					" <column-description>",
					"   <column name='command' description='Full command name used to invoke the function' />",
					"   <column name='description' description='Command help description' />",
					" </column-description>",
					" <content></content>",
					"</table>", NULL);
	/* check document created */
	if (result == NULL) {
		error ("Failed to produce base table output for commands available, error was: %d:%s",
		       axl_error_get_code (err), axl_error_get (err));
		axl_error_free (err);
		return NULL;
	} /* end if */

	/* now get a reference to the content to introduce content */
	node   = axl_doc_get (result, "/table/content");

	/* create the cursor */
	while (iterator < axl_list_length (commands)) {
		/* get next node */
		item    = axl_list_get_nth (commands, iterator);
		
		/* create command item */
		nodeAux = axl_node_parse (NULL, "<row><d><![CDATA[%s]]></d><d><![CDATA[%s]]></d></row>",
					  item->command, item->description);
		
		/* set child */
		axl_node_set_child (node, nodeAux);

		/* next iterator */
		iterator++;
	}

	/* unlock */
	vortex_mutex_unlock (&commands_mutex);

	/* return document */
	return result;
}


/** 
 * @internal Function used to install commands into the list of
 * commands available.
 */
void mod_radmin_install_command (const char               * command, 
				 const char               * description,
				 ModRadminCommandHandler    handler,
				 axlPointer                 user_data)
{
	ModRadminCommandItem * cmd;

	/* lock mutex */
	vortex_mutex_lock (&commands_mutex);

	/* create command handler */
	cmd              = axl_new (ModRadminCommandItem, 1);
	cmd->command     = axl_strdup (command);
	cmd->length      = strlen (command);
	cmd->description = axl_strdup (description);
	cmd->handler     = handler;

	/* install it */
	axl_list_append (commands, cmd);

	/* unlock mutex */
	vortex_mutex_unlock (&commands_mutex);
	
	return;
}

/** 
 * @internal Function that install default support commands for remote
 * admin.
 */
void mod_radmin_install_default_commands (void) {
	
	mod_radmin_install_command ("reload", 
				    "Allows to instruct turbulence to reload its configuration", 
				    mod_radmin_command_reload, NULL);
	mod_radmin_install_command ("status", 
				    "Allows to report status", 
				    mod_radmin_command_status, NULL);
	mod_radmin_install_command ("kill conn", 
				    "Allows to terminate the provided connection on the selected process:\n               Usage:\n                 > kill conn [pid:conn-id]\n                 Get connection ids (conn-id) and pids using:\n                 > show connections", 
				    mod_radmin_command_kill_connection, NULL);
	mod_radmin_install_command ("show connections",
				    "Allows to get all connections being handled by turbulence at this moment", 
				    mod_radmin_command_show_connections, NULL);
	mod_radmin_install_command ("show channels",
				    "Allows to get all channels being handled by turbulence at this moment", 
				    mod_radmin_command_show_channels, NULL);
	mod_radmin_install_command ("kill child", 
				    "Allows to terminate the child identified with the provided pid:\n               Usage:\n                 > kill child [pid]\n                 Get child pids using:\n                 > show childs", 
				    mod_radmin_command_kill_child, NULL);
	mod_radmin_install_command ("show childs", 
				    "Allows to list of turbulence child processes", 
				    mod_radmin_command_show_childs, NULL);
	mod_radmin_install_command ("commands available",
				    "Returns the list of commands available at the moment the request is executed",
				    mod_ramdin_command_commands_available, NULL);
	return;
}

/** 
 * @internal Function that handles incoming requests for
 * mod-radmin/command-install api calls.
 */
void mod_radmin_command_install_aux (TurbulenceMediatorObject * object)
{
	/* get command, description and handler */
	const char               * command     = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);
	const char               * description = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA2);
	ModRadminCommandHandler    handler     = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA3);
	axlPointer                 user_data   = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA4);

	msg ("received request to install command");
	mod_radmin_install_command (command, description, handler, user_data);
	
	return;
}

void mod_radmin_frame_received (VortexChannel    * channel,
				VortexConnection * conn,
				VortexFrame      * frame,
				axlPointer         user_data)
{
	axlDoc   * doc;
	axlError * error = NULL;

	/* check we only receive MSG frame request */
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_MSG)  {
		/* close the connection */
		vortex_connection_shutdown (conn);
		return;
	} /* end if */

	/* command received, handle request and reply */
	/* msg ("received command request: %s", vortex_frame_get_payload (frame)); */

	/* check command received */
	doc = axl_doc_parse ((const char *) vortex_frame_get_payload (frame), vortex_frame_get_payload_size (frame), &error);
	if (doc == NULL) {
		vortex_channel_send_errv (channel, 
					  vortex_frame_get_msgno (frame), "<error code='%d'><msg><![CDATA[%s]]></msg><content>Unable to process command received, a failure in XML document parsing was found: (%d:%s). Content received was: <![CDATA[%s]]></content></error>", 
					  axl_error_get_code (error),
					  axl_error_get (error),
					  axl_error_get_code (error),
					  axl_error_get (error),
					  vortex_frame_get_payload (frame));
		axl_error_free (error);
		return;
	} /* end if */

	/* check command received */
	mod_radmin_handle_command (conn, channel, doc, frame);

	/* release document */
	axl_doc_free (doc);
	
	return;
}

void mod_radmin_child_kill_conn (TurbulenceCtx    * ctx, 
				 const char       * command, 
				 VortexConnection * caller_conn, 
				 VortexChannel    * channel, 
				 VortexFrame      * frame)
{
	int                conn_id = atoi (command + 9);
	VortexConnection * conn;
	axlDoc           * doc;

	msg ("Received request from parent to kill a particular connection id=%d, starting..", conn_id);

	/* find connection */
	conn = turbulence_conn_mgr_find_by_id (ctx, conn_id);
	if (conn) {
		/* found connection */
		vortex_connection_shutdown (conn);

		/* now handle reply */
		doc = mod_radmin_ok_msg (550, "Connection killed", "Connection close initiated on child process");
		mod_radmin_handle_command_reply (axl_true, doc, caller_conn, channel, frame);
		
		return;
	} /* end if */

	return;
}

/** 
 * @internal Handler called a request from parent is received through
 * the child conn mgr.
 */
void mod_radmin_internal_frame_received  (VortexChannel    * channel,
					  VortexConnection * conn,
					  VortexFrame      * frame,
					  axlPointer         user_data)
{
	TurbulenceCtx * ctx = user_data;
	axlDoc        * doc;
	axl_bool        status = axl_false;
	const char    * command;

	/* get reference to the command received */
	command = (const char *) vortex_frame_get_payload (frame);

	/* check connection where request is received is child conn
	 * mgr */
	if (vortex_connection_get_id (conn) != vortex_connection_get_id (ctx->child->conn_mgr)) {
		error ("CHILD: received frame for internal handler through a connection id=%d that is not child conn id=%d mgr, shutting down",
		       vortex_connection_get_id (conn), vortex_connection_get_id (ctx->child->conn_mgr));
		vortex_connection_shutdown (conn);
		return;
	} /* end if */
	
	/* check command */
	if (axl_cmp ("show connections", command)) {
		/* reuse function */
		doc = mod_radmin_command_show_connections (NULL, NULL, &status);

		/* now handle reply */
		mod_radmin_handle_command_reply (status, doc, conn, channel, frame);
	} else if (axl_cmp ("show channels", command)) {
		/* reuse function */
		doc = mod_radmin_command_show_channels (NULL, NULL, &status);

		/* now handle reply */
		mod_radmin_handle_command_reply (status, doc, conn, channel, frame);
	} else if (axl_cmp ("kill child", command)) {
		msg ("Received request from parent to kill current process, starting..");
		/* unlock the current listener */
		vortex_listener_unlock (TBC_VORTEX_CTX (ctx));
		
		/* no need for reply because the process will end */
	} else if (axl_memcmp ("kill conn", command, 9)) {

		/* handle connection close */
		mod_radmin_child_kill_conn (ctx, command, conn, channel, frame);
		
	} /* end if */

	return;
}
	
/* mod_radmin init handler */
static int  mod_radmin_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* check if we are master or child process */
	if (! ctx->child) {

		/* init commands support */
		commands = axl_list_new (axl_list_always_return_1, mod_radmin_command_item_free);
		vortex_mutex_create (&commands_mutex);
	
		/* install default commands */
		mod_radmin_install_default_commands ();

		/* publish an api to accept incoming requests to install
		   commands */
		turbulence_mediator_create_api (ctx, "mod-radmin", "command-install", 
						/* handler */
						mod_radmin_command_install_aux, NULL);
		
		/* register BEEP profile to receive commands */
		vortex_profiles_register (turbulence_ctx_get_vortex_ctx (ctx),
					  RADMIN_URI,
					  /* no start handler for now */
					  NULL, NULL,
					  /* no close handler for now */
					  NULL, NULL,
					  /* frame received */
					  mod_radmin_frame_received, NULL);
	} else {
		msg ("mod-radmin: running init at child..");
		/* we are a child, so register internal channel to internal
		 * master->child requests */
		vortex_profiles_register (TBC_VORTEX_CTX (ctx), RADMIN_URI_INTERNAL,
					  NULL, NULL, NULL, NULL, mod_radmin_internal_frame_received, _ctx);
	}

	/* initialization ok */
	return axl_true;
} /* end mod_radmin_init */

/* mod_radmin close handler */
static void mod_radmin_close (TurbulenceCtx * _ctx) {
	
	/* terminate commands */
	axl_list_free (commands);

	/* terminate mutex */
	vortex_mutex_destroy (&commands_mutex);

	/* unregister profile */
	vortex_profiles_unregister (turbulence_ctx_get_vortex_ctx (_ctx), RADMIN_URI);

	return;
} /* end mod_radmin_close */

/* mod_radmin reconf handler */
static void mod_radmin_reconf (TurbulenceCtx * _ctx) {
	
	return;
} /* end mod_radmin_reconf */

/* mod_radmin unload handler */
static void mod_radmin_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_radmin_unload */

axl_bool mod_radmin_ppath_selected (TurbulenceCtx * ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn)
{


	return axl_true; /* always return true to avoid closing current child */
}

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_radmin",
	"Remote administration and status checking for Turbulence",
	mod_radmin_init,
	mod_radmin_close,
	mod_radmin_reconf,
	mod_radmin_unload,
	mod_radmin_ppath_selected
};

/** 
 * \page turbulence_mod_radmin mod-radmin: Administrative and status module for turbulence
 *
 * \section turbulence_mod_radmin_intro Introduction
 *
 * mod-radmin is a C turbulence module that provides administrators
 * the hability to check internal turbulence status and to do some
 * administrative tasks like reloading.
 *
 * \section turbulence_mod_radmin_configuration Fast mod-radmin module configuration
 *
 * The following allows to setup a secure mod-radmin configuration
 * that will allow root users to connect to local server.
 *
 * To do, so just run the following script bundled with turbulence. It
 * is assumed you have turbulence installed on the system. 
 *
 * \code
 * >> tbc-setup-mod-radmin.py
 * \endcode
 *
 * After this command is successfully run (follow all instructions), \ref turbulence_mod_radmin_using "you can start turbulence-ctl as usual".
 *
 * \section turbulence_mod_radmin_configuration Configuring mod-radmin module (server side), long version
 *
 * After enabling the module (see \ref turbulence_modules_activation),
 * you need to configure a profile path inside turbulence.conf file to
 * allow the profile used by mod-radmin from the locations you
 * want. 
 *
 * Maybe the simpliest way to enable mod-radmin is to allow its usage
 * without passwords only from localhost. <b>This is the best method when
 * developing, but it is really insecure when using Turbulence in
 * production/hostile environment</b> (even only allowing connections from
 * localhost, because that address is accesible to code that runs
 * Turbulence). 
 *
 * Here is how to enable mod-radmin for localhost. Add the following
 * profile path declaration inside <b>&lt;profile-path-configuration></b> node, found in turbulence.conf file:
 *
 * \code
 *   <path-def server-name="localhost" src="127.0.0.1" path-name="local radmin">
 *     <allow profile="urn:aspl.es:beep:profiles:radmin-ctl" />
 *   </path-def>
 * \endcode
 *
 * You must ensure this is the first declaration that matches
 * serverName="localhost". If you want to be able to connect with the
 * address (127.0.0.1), change server-name declaration.
 *
 * A more secure mod-radmin configuration will be to use TLS to secure
 * the connection, and SASL to ask for a password before allowing
 * mod-radmin. This method is the recommended for production because
 * allows connecting from any place. Here is how to enable mod-radmin
 * with TLS+SASL:
 *
 * \code
 *   <path-def server-name="radmin.yourserver.com" src=".*" path-name="remote radmin">
 *     <if-success profile="http://iana.org/beep/TLS" >
 *        <if-success profile="http://iana.org/beep/SASL/.*" connmark="sasl:is:authenticated" >
 *           <allow profile="urn:aspl.es:beep:profiles:radmin-ctl" />
 *        </if-success>
 *     </if-success>
 *   </path-def>
 * \endcode
 * 
 * NOTE: check \ref turbulence_mod_sasl "mod-sasl documenation" to
 * know how to configure a user database that authenticates
 * connections for mod-radmin and \ref turbulence_mod_tls "mod-tls
 * documentation" to know how to configure the certificate and key to
 * use to secure the connection.
 *
 * IMPORTANT NOTE: You must configure the profile path that holds
 * mod-radmin without separate="yes". This is because the module must
 * execute in the parent process. 
 *
 * If you configure separate="yes", the module will be activated on
 * child process, making registered profile to be not available (child
 * process can't receive connections, only parents do).
 *
 * 
 * \section turbulence_mod_radmin_using Using mod-radmin (client side)
 *
 * Assuming you have a Turbulence with mod-radmin enabled, you just
 * connect with:
 *
 * \code
 * >> turbulence-ctl
 * I: connecting turbulence at localhost:602..
 * I: connected OK!
 * tbc-ctl:localhost:602> 
 * \endcode
 *
 * Now get connections available at this moment and its activity:
 *
 * \code
 * tbc-ctl:localhost:602> show connections
 * >>> Connection list <<<
 * 
 * proc-id   conn-id   role       source            dest            channels opened   ppath          auth id   last activity              bytes recv   bytes sent   
 * -------   -------   --------   ---------------   -------------   ---------------   ------------   -------   ------------------------   ----------   ----------   
 * 28693     1         master     0.0.0.0:602       0.0.0.0:602     -1                -              -         -                          0            0            
 * 28693     2         master     0.0.0.0:604       0.0.0.0:604     -1                -              -         -                          0            0            
 * 28693     3         master     0.0.0.0:3206      0.0.0.0:3206    -1                -              -         -                          0            0            
 * 28693     7         listener   127.0.0.1:43024   127.0.0.1:602   2                 local radmin   -         Tue Aug 23 17:18:10 2011   377          1711         
 * 28702     7         listener   127.0.0.1:43022   127.0.0.1:602   3                 core-admin     -         Tue Aug 23 17:17:58 2011   1041         603     
 * \endcode
 *
 * Write "help" or press to autocomplete two times to get commands autocompleted.
 *
 * \section turbulence_mod_radmin_problems Usual problems found while using mod-radmin
 *
 * <b>Why I don't see connections or childs?</b>
 *
 * Check you you have declared profile path supporting mod-radmin
 * without using separate="yes". 
 *
 * 
 *
 * 
 */

END_C_DECLS


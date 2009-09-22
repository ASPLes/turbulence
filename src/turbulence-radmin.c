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
#include <turbulence-radmin.h>
#include <sys/un.h>
#include <stddef.h>

/* include local private ctx definition */
#include <turbulence-ctx-private.h>

int turbulence_radmin_create_file_socket (TurbulenceCtx * ctx, const char * filename)
{
	struct sockaddr_un name;
	int sock;
	size_t size;
	
	/* Create the socket. */
	sock = socket (PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		error ("socket() call failed while creating file socket");
		CLEAN_START (ctx);
		return -1;
	}

	/* Bind a name to the socket. */
	name.sun_family = AF_FILE;
	strcpy (name.sun_path, filename);

	/* The size of the address is the offset of the start of the
	   filename, plus its length, plus one for the terminating
	   null byte. */
	size = (offsetof (struct sockaddr_un, sun_path) + strlen (name.sun_path) + 1);

	if (bind (sock, (struct sockaddr *) &name, size) < 0) {
		error ("bind() call failed while creating file socket, errno: %d, %s (file=%s)",
		       errno, vortex_errno_get_last_error (), filename);
		CLEAN_START(ctx);
		return -1;
	}

	return sock;
}

int turbulence_radmin_file_socket_connect (TurbulenceCtx * ctx, const char * filename)
{
	struct sockaddr_un name;
	int sock;
	size_t size;
	
	/* Create the socket. */
	sock = socket (PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		error ("socket() call failed while creating file socket");
		CLEAN_START (ctx);
		return -1;
	}

	/* Bind a name to the socket. */
	name.sun_family = AF_FILE;
	strcpy (name.sun_path, filename);

	/* The size of the address is the offset of the start of the
	   filename, plus its length, plus one for the terminating
	   null byte. */
	size = (offsetof (struct sockaddr_un, sun_path) + strlen (name.sun_path) + 1);

	if (connect (sock, (struct sockaddr *) &name, size) < 0) {
		error ("connect() call failed while creating file socket, errno: %d, %s (file=%s)",
		       errno, vortex_errno_get_last_error (), filename);
		CLEAN_START(ctx);
		return -1;
	}

	return sock;
}

typedef struct _TurbulenceRadminState {
	axl_bool auth_complete;
	int      tries;
} TurbulenceRadminState;

axl_bool turbulence_radmin_read_content (TurbulenceLoop * loop,
					 TurbulenceCtx  * ctx,
					 int              descriptor,
					 axlPointer       ptr,
					 axlPointer       ptr2)
{
	TurbulenceRadminState * state = (TurbulenceRadminState *) ptr;
	char                    buffer[1024];
	int                     content;

	msg ("received content from client management connection");

	/* read content */
	content = read (descriptor, buffer, 1023);
	if (content == 0) {
		msg ("detected client management connection close");
		return axl_false;
	} /* end if */

	buffer[content] = 0;
	msg ("content received: '%s'", buffer);

	if (! state->auth_complete) {
		/* check user and password */

		/* check tries if it failed */
	}
	
	/* continue with this socket */
	return axl_true;
}

axl_bool turbulence_radmin_init_client (TurbulenceLoop * loop,
					TurbulenceCtx  * ctx,
					int              descriptor,
					axlPointer       ptr,
					axlPointer       ptr2)
{
	/* accept client socket and register it waiting to authenticate */
	TurbulenceRadminState * state;
	VORTEX_SOCKET           client = vortex_listener_accept (descriptor);

	msg ("accepting client local management (%d)", client);

	/* if an error is found do not continue */
	if (client == VORTEX_SOCKET_ERROR)
		return axl_true;
	
	/* create initial state */
	state                = axl_new (TurbulenceRadminState, 1);
	state->auth_complete = axl_false;
	state->tries         = 3;

	/* register socket */
	turbulence_loop_watch_descriptor (loop, client, 
					  turbulence_radmin_read_content,
					  state, NULL);
	/* register the state to release its memory once closed the
	   socket: the following will release the state either because
	   a second connection is received with the same socket value
	   or because the context is closed. */
	turbulence_ctx_set_data_full (ctx, axl_strdup_printf ("%d", client), state,
				      axl_free, axl_free);
	return axl_true;
}


/** 
 * @internal Inits turbulence local socket used for remote local
 * turbulence administration (-r option).
 */
axl_bool turbulence_radmin_init (TurbulenceCtx * ctx)
{
	axlNode    * node;
	axlDoc     * doc;
	const char * user;
	const char * group;
	const char * mode;

	/* initialize socket */
	ctx->management_socket = -1;

	/* check if local management is enabled */
	doc  = turbulence_config_get (ctx);
	node = axl_doc_get (doc, "/turbulence/global-settings/local-management");
	
	if (turbulence_config_is_attr_negative (ctx, node, "enabled")) {
		msg ("turbulence local management interface not enabled, skipping");
		return axl_true;
	} /* end if */

	/* seems management interface is enabled: activate it */
	node = axl_node_get_child_called (node, "file-socket");
	
	/* open file socket */
	ctx->management_socket = turbulence_radmin_create_file_socket (ctx, ATTR_VALUE (node, "value"));
	if (ctx->management_socket == -1) 
		return axl_true; /* return axl_true because the function already calls to CLEAN_START */
	msg ("turbulence local management file socket: %s", ATTR_VALUE (node, "value"));

	/* change owner and permissions */
	if (HAS_ATTR (node, "user") || HAS_ATTR (node, "group")) {
		/* get owner values */
		user  = ATTR_VALUE (node, "user");
		group = ATTR_VALUE (node, "group");
		
		/* do the change */
		if (! turbulence_change_fd_owner (ctx, ATTR_VALUE (node, "value"), user, group)) {
			error ("failed to change local management socket owner (user:%s, group: %s), errno: %d, %s",
			       user ? user : "", group ? group : "", errno, vortex_errno_get_last_error ());
			CLEAN_START (ctx);
		} /* end if */
	} /* end if */

	/* now check mode if defined */
	if (HAS_ATTR (node, "mode")) {
		/* get mode values */
		mode = ATTR_VALUE (node, "mode");
		if (! turbulence_change_fd_perms (ctx, ATTR_VALUE (node, "value"), mode)) {
			error ("failed to change local management socket mode (mode: %s), errno: %d, %s",
			       mode ? mode : "", errno, vortex_errno_get_last_error ());
			CLEAN_START (ctx);
		} /* end if */
	} /* end if */

	/* start admin loop */
	msg ("started local management interface loop");
	ctx->radmin_loop = turbulence_loop_create (ctx);

	/* watch listener */
	turbulence_loop_watch_descriptor (ctx->radmin_loop,
					  ctx->management_socket,
					  turbulence_radmin_init_client,
					  NULL, NULL);
	return axl_true;
}

/** 
 * @internal Function used to handle content received from server
 * component.
 */
axl_bool turbulence_radmin_client_handle (TurbulenceLoop * loop,
					 TurbulenceCtx  * ctx,
					 int              descriptor,
					 axlPointer       ptr,
					 axlPointer       ptr2)
{
	return axl_true;
}

/** 
 * @internal Function used to handle content received from standard
 * input, to be sent to the server component.
 */
axl_bool turbulence_radmin_read_client_input  (TurbulenceLoop * loop,
					       TurbulenceCtx  * ctx,
					       int              descriptor,
					       axlPointer       ptr,
					       axlPointer       ptr2)
{
	VortexAsyncQueue * queue  = (VortexAsyncQueue *) ptr; 
	int                socket = PTR_TO_INT (ptr2); 
	char               buffer[1024];
	int                bytes;

	/* read content */
	bytes = read (descriptor, buffer, 1023);
	buffer[bytes] = 0;

	/* remove trailing \n */
	if (bytes > 0 && buffer[bytes - 1] == '\n') {
		buffer[bytes - 1] = 0;
		bytes -= 1;
	}

	/* check quit command */
	if (axl_cmp (buffer, "quit") || axl_cmp (buffer, "exit")) {
		vortex_async_queue_push (queue, INT_TO_PTR (1));
		return axl_false;
	}

	/* print prompt */
	printf ("CLI*turbulence> ");
	fflush (stdout);

	/* now send content to socket */
	msg ("Sending content over socket %d (bytes: %d)", socket, bytes);
	bytes = write (socket, buffer, bytes);

	return axl_true;
}

/** 
 * @brief Performs a client local connect to local process using the
 * file socket found at the configuration file.
 *
 * @param ctx The turbulence context.
 * @param config The configuration location.
 *
 */
void turbulence_radmin_client_connect (TurbulenceCtx * ctx, 
				       const char    * config)
{
	VORTEX_SOCKET      socket;
	axlNode          * node;
	VortexAsyncQueue * queue;

	/* load configuration file */
	if (! turbulence_config_load (ctx, config)) 
		return;

	/* find socket location */
	node = axl_doc_get (turbulence_config_get (ctx), "/turbulence/global-settings/local-management");
	if (turbulence_config_is_attr_negative (ctx, node, "enabled")) {
		error ("turbulence local management interface not enabled, unable to connect");
		return;
	} /* end if */
	
	/* seems management interface is enabled: connect using file socket */
	node = axl_node_get_child_called (node, "file-socket");
	
	/* connect */
	socket = turbulence_radmin_file_socket_connect (ctx, ATTR_VALUE (node, "value"));
	if (socket == -1) {
		error ("unable to connect to local management, errno %d:%s", errno, vortex_errno_get_last_error ());
		return; /* return axl_true because the function already calls to CLEAN_START */
	}
	msg ("connected to local management file socket: %s (socket: %d)", ATTR_VALUE (node, "value"), socket);

	/* print prompt */
	printf ("CLI*turbulence> ");
	fflush (stdout);

	/* create loop to control client stuff */
	ctx->radmin_loop = turbulence_loop_create (ctx);
	
	/* watch client */
	queue = vortex_async_queue_new ();
	turbulence_loop_watch_descriptor (ctx->radmin_loop, socket, turbulence_radmin_client_handle, queue, NULL);
	turbulence_loop_watch_descriptor (ctx->radmin_loop, 1, turbulence_radmin_read_client_input, queue, INT_TO_PTR (socket));

	/* pop to lock */
	vortex_async_queue_pop (queue);
	vortex_async_queue_unref (queue);

	/* finish loop */
	turbulence_loop_close (ctx->radmin_loop, axl_true);
	ctx->radmin_loop = NULL;

	return;
}

/** 
 * @brief Allows to cleanup file and socket created.
 */
void turbulence_radmin_cleanup (TurbulenceCtx * ctx)
{
	/* get location */
	axlNode * node;

	if (ctx->management_socket == -1)
		return;

	/* get socket file location */
	node = axl_doc_get (turbulence_config_get (ctx), "/turbulence/global-settings/local-management/file-socket");

	/* close socket */
	vortex_close_socket (ctx->management_socket);
	turbulence_loop_close (ctx->radmin_loop, axl_true);

	/* remove file */
	turbulence_unlink (ATTR_VALUE (node, "value"));

	return;
}



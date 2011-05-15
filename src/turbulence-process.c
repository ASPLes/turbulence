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
#include <turbulence-process.h>

/* include private headers */
#include <turbulence-ctx-private.h>

/* include signal handler: SIGCHLD */
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>

#if !defined(random)
long random (void);
#endif

void turbulence_process_free_child_data (TurbulenceChild * child)
{
	if (child == NULL)
		return;
#if defined(AXL_OS_UNIX)
	/* unlink (child->socket_control_path);*/
	axl_free (child->socket_control_path);
	child->socket_control_path = NULL;
	vortex_close_socket (child->child_connection);
#endif

	axl_free (child);
	return;
}

/** 
 * @internal Function used to init process module (for its internal
 * handling).
 *
 * @param ctx The turbulence context where the initialization will
 * take place.
 */
void turbulence_process_init         (TurbulenceCtx * ctx, axl_bool reinit)
{
	/* check for reinit operation */
	if (reinit && ctx->child_process) {
		axl_hash_free (ctx->child_process);
		ctx->child_process = NULL;
	}

	/* create the list of childs opened */
	if (ctx->child_process == NULL)
		ctx->child_process = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* init mutex */
	vortex_mutex_create (&ctx->child_process_mutex);
	return;
}


typedef struct _TurbulenceProcessFinished {
	VortexCtx       * vortex_ctx;
	TurbulenceChild * child;
} TurbulenceProcessFinished;

axlPointer __turbulence_process_finished (TurbulenceProcessFinished * data)
{
	TurbulenceChild  * child      = data->child;
	VortexCtx        * vortex_ctx = data->vortex_ctx;
	TurbulenceCtx    * ctx        = child->ctx;
	int                delay      = 300000; /* 300ms */
	int                tries      = 10; 

	/* free pointer received */
	axl_free (data);
	
	while (tries > 0) {

		/* check if the reader has connections watched at this time */
		if (vortex_reader_connections_watched (vortex_ctx) > 0) {
			msg2 ("cancelled child process termination because new connections are now handled, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d",
			      tries, delay, 
			      vortex_reader_connections_watched (vortex_ctx), 
			      axl_hash_items (ctx->conn_mgr_hash));
			return NULL;
		}
		
		/* finish current thread if turbulence is existing */
		if (ctx->is_exiting) {
			msg2 ("Found turbulence existing, finishing child process termination thread signaled due to vortex reader stop");
			return NULL;
		}

		/* reduce tries, wait */
		msg2 ("delay child process termination to ensure the parent has no pending connections, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d, is-existing=%d",
		      tries, delay,
		      vortex_reader_connections_watched (vortex_ctx), 
		      axl_hash_items (ctx->conn_mgr_hash),
		      ctx->is_exiting);
		tries--;
		turbulence_sleep (ctx, delay);
	}

	/* unlock waiting child */
	msg2 ("calling to unlock due to vortex reader stoped (no more connections to be watched): %p (%p)..",
	      ctx, ctx->child_wait);
	if (ctx && ctx->child_wait) {
		vortex_async_queue_push (ctx->child_wait, INT_TO_PTR (axl_true));
	}
	return NULL;
}


void turbulence_process_finished (VortexCtx * vortex_ctx, axlPointer user_data)
{
	VortexThread                thread;
	TurbulenceProcessFinished * data;
	TurbulenceChild           * child = user_data;
	TurbulenceCtx             * ctx   = child->ctx;

	/* do not implement any notification if the turbulence process
	   is itself finishing */
	if (ctx->is_exiting)
		return;

	/* check if the child process was configured with a reuse
	   flag, if not, notify exist right now */
	if (! child->ppath->reuse) {
		/* so it is a child process without reuse flag
		   activated, exit now */
		if (ctx && ctx->child_wait) {
			vortex_async_queue_push (ctx->child_wait, INT_TO_PTR (axl_true));
		} /* end if */
		return;
	}

	/* reached this point, the child process has reuse = yes which
	   means we have to ensure we don't miss any connection which
	   is already accepted on the parent but still not reached the
	   child: the following creates a thread to unlock the vortex
	   reader, so he can listen for new registration while we do
	   the wait code for new connections... */

	/* build data to be notified */
	data             = axl_new (TurbulenceProcessFinished, 1);
	data->vortex_ctx = vortex_ctx;
	data->child      = user_data;
	
	/* create data to be notified */
	if (! vortex_thread_create (&thread,
				    (VortexThreadFunc) __turbulence_process_finished,
				    data, VORTEX_THREAD_CONF_END)) {
		error ("Failed to create thread to watch reader state and notify process termination (code %d) %s",
		       errno, vortex_errno_get_last_error ());
	}
	return;
}

/** 
 * @internal Reference to the context used by the child process inside
 * turbulence_process_create_child.
 */
TurbulenceCtx * child_ctx   = NULL;

void turbulence_process_signal_received (int _signal) {
	/* default handling */
	turbulence_signal_received (child_ctx, _signal);
	return;
}

#if defined(AXL_OS_UNIX)
int __turbulence_process_local_unix_fd (const char *path, axl_bool is_parent, TurbulenceCtx * ctx)
{
	int                  _socket     = -1;
	int                  _aux_socket = -1;
	struct sockaddr_un   socket_name = {0};
	int                  tries       = 10;
	int                  delay       = 100;

	/* configure socket name to connect to (or to bind to) */
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	strcpy (socket_name.sun_path, path);
	socket_name.sun_family = AF_UNIX;

	/* create socket and check result */
	_socket = socket (AF_UNIX, SOCK_STREAM, 0);
	if (_socket == -1) {
	        error ("%s: Failed to create local socket to hold connection, reached limit?, errno: %d, %s", 
		       is_parent ? "PARENT" : "CHILD", errno, vortex_errno_get_error (errno));
	        return -1;
	}

	/* if child, wait until it connects to the child */
	if (is_parent) {
		while (tries > 0) {
			if (connect (_socket, (struct sockaddr *)&socket_name, sizeof (socket_name))) {
				if (errno == 107 || errno == 111 || errno == 2) {
					/* implement a wait operation */
					turbulence_sleep (ctx, delay);
				} else {
					error ("%s: Unexpected error found while creating child control connection: (code: %d) %s", 
					       is_parent ? "PARENT" : "CHILD", errno, vortex_errno_get_last_error ());
					break;
				} /* end if */
			} else {
				/* connect == 0 : connected */
				break;
			}

			/* reduce tries */
			tries--;
			delay = delay * 2;

		} /* end if */

		/* drop an ok log */
		msg ("%s: local socket (%s) = %d created OK", is_parent ? "PARENT" : "CHILD", path, _socket);

		return _socket;
	}

	/* child handling */
	/* unlink for previously created file */
	unlink (path);

	/* check path */
	if (strlen (path) >= sizeof (socket_name.sun_path)) {
	        vortex_close_socket (_socket);
	        error ("%s: Failed to create local socket to hold connection, path is bigger that limit (%d >= %d), path: %s", 
		       is_parent ? "PARENT" : "CHILD", strlen (path), sizeof (socket_name.sun_path), path);
		return -1;
	}
	umask (0077);
	if (bind (_socket, (struct sockaddr *) &socket_name, sizeof(socket_name))) {
	        error ("%s: Failed to create local socket to hold conection, bind function failed with error: %d, %s", 
		       is_parent ? "PARENT" : "CHILD", errno, vortex_errno_get_last_error ());
		vortex_close_socket (_socket);
		return -1;
	} /* end if */

	/* listen */
	if (listen (_socket, 1) < 0) {
	        vortex_close_socket (_socket);
		error ("%s: Failed to listen on socket created, error was (code: %d) %s", 
		       is_parent ? "PARENT" : "CHILD", errno, vortex_errno_get_last_error ());
		return -1;
	}

	/* now call to accept connection from parent */
	_aux_socket = vortex_listener_accept (_socket);
	if (_aux_socket < 0) 
	         error ("%s: Failed to create local socket to hold conection, listener function failed with error: %d, %s (socket value is %d)", 
			is_parent ? "PARENT" : "CHILD", errno, vortex_errno_get_last_error (), _aux_socket);
	vortex_close_socket (_socket);

	/* unix semantic applies, now remove the file socket because
	 * now having opened a socket, the reference will be removed
	 * after this is closed. */
	unlink (path);

	/* drop an ok log */
	msg ("%s: local socket (%s) = %d created OK", is_parent ? "PARENT" : "CHILD", path, _socket);	
	
	return _aux_socket;
}
#endif

axl_bool __turbulence_process_create_child_connection (TurbulenceChild * child)
{
	TurbulenceCtx    * ctx = child->ctx;
	struct sockaddr_un socket_name = {0};

	/* configure socket name to connect to (or to bind to) */
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	strcpy (socket_name.sun_path, child->socket_control_path);
	socket_name.sun_family = AF_UNIX;

	/* create the client connection making the child to do the
	   bind (creating local file socket using child process
	   permissions)  */
	child->child_connection = __turbulence_process_local_unix_fd (child->socket_control_path, axl_true, ctx);
	return (child->child_connection > 0);
}

axl_bool __turbulence_process_create_parent_connection (TurbulenceChild * child)
{
	/* create the client connection making the child to do the
	   bind (creating local file socket using child process
	   permissions)  */
	child->child_connection = __turbulence_process_local_unix_fd (child->socket_control_path, axl_false, child->ctx);
	return (child->child_connection > 0);
}

/** 
 * @internal Function used to send the provided socket to the provided
 * child.
 *
 * @param socket The socket to be send.
 *
 * @param child The child 
 *
 * @param wait_confirmation Signals if the caller that is sending the
 * socket must wait for confirmation from the receiving side.
 *
 * @return If the socket was sent.
 */
axl_bool turbulence_process_send_socket (VORTEX_SOCKET     socket, 
					 TurbulenceChild * child, 
					 const char      * ancillary_data, 
					 int               size)
{
	struct msghdr        msg;
	struct sockaddr_un   socket_name;
	char                 ccmsg[CMSG_SPACE(sizeof(socket))];
	struct cmsghdr     * cmsg;
	struct iovec         vec; 
	/* send at least one byte */
	const char         * str = ancillary_data ? ancillary_data : "#"; 
	int                  rv;
	TurbulenceCtx      * ctx = child->ctx;

	/* configure socket name to send the socket to */
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	strcpy (socket_name.sun_path, child->socket_control_path);
	socket_name.sun_family = AF_UNIX;

	/* clear structures */
	memset (&msg, 0, sizeof (struct msghdr));

	/* configure destination */
	/* msg.msg_name           = (struct sockaddr*)&socket_name;
	   msg.msg_namelen        = sizeof (socket_name);   */
	msg.msg_namelen        = 0;

	vec.iov_base   = (char *) str;
	vec.iov_len    = ancillary_data == NULL ? 1 : size;
	msg.msg_iov    = &vec;
	msg.msg_iovlen = 1;

	msg.msg_control        = ccmsg;
	msg.msg_controllen     = sizeof(ccmsg);
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level       = SOL_SOCKET;
	cmsg->cmsg_type        = SCM_RIGHTS;
	cmsg->cmsg_len         = CMSG_LEN(sizeof(socket));
	*(int*)CMSG_DATA(cmsg) = socket;

	msg.msg_controllen     = cmsg->cmsg_len;
	msg.msg_flags = 0;
	
	rv = (sendmsg (child->child_connection, &msg, 0) != -1);
	if (rv)  {
		msg ("Socket %d sent to child via %d, closing (status: %d)..", 
		     socket, child->child_connection, rv);
		/* close the socket  */
		vortex_close_socket (socket);
	} else {
		error ("Failed to send socket, error was: %s", vortex_errno_get_error (errno));
	}
	
	return rv;
}

/** 
 * @brief Allows to receive a socket from the parent on the child
 * provided. In the case the function works, the socket references is
 * updated with the socket descriptor.
 *
 * @param socket A reference where to set socket received. It cannot be NULL.
 * @param child Child receiving the socket. 
 *
 * @return The function returns axl_false in the case of failure,
 * otherwise axl_true is returned.
 */
axl_bool turbulence_process_receive_socket (VORTEX_SOCKET    * socket, 
					    TurbulenceChild  * child, 
					    char            ** ancillary_data, 
					    int              * size)
{
	struct msghdr    msg;
	struct iovec     iov;
	char             buf[8192];
	int              status;
	char             ccmsg[CMSG_SPACE(sizeof(int))];
	struct           cmsghdr *cmsg;
	TurbulenceCtx  * ctx;
	
	v_return_val_if_fail (socket && child, axl_false);

	/* get context reference */
	ctx = child->ctx;
	
	iov.iov_base = buf;
	iov.iov_len = 8192;
	
	msg.msg_name       = 0;
	msg.msg_namelen    = 0;
	msg.msg_iov        = &iov;
	msg.msg_iovlen     = 1;
	msg.msg_control    = ccmsg;
	msg.msg_controllen = sizeof(ccmsg); 
	
	status = recvmsg (child->child_connection, &msg, 0);
	if (status == -1) {
		error ("Failed to receive socket, recvmsg failed, error was: (code %d) %s",
		       errno, vortex_errno_get_last_error ());
		(*socket) = -1;
		return axl_false;
	} /* end if */

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		error ("Received empty control message from parent, unable to receive socket (code %d): %s",
		       errno, vortex_errno_get_last_error ());
		(*socket) = -1;
		if (ancillary_data)
			(*ancillary_data) = NULL;
		if (size)
			(*size) = 0;
		return axl_false;
	}

	if (!cmsg->cmsg_type == SCM_RIGHTS) {
		error ("Unexpected control message of unknown type %d, failed to receive socket", 
		       cmsg->cmsg_type);
		(*socket) = -1;
		return axl_false;
	}

	/* report data received on the message */
	if (size)
		(*size)   = status;
	if (ancillary_data) {
		(*ancillary_data) = axl_new (char, status + 1);
		memcpy (*ancillary_data, iov.iov_base, status);
	}

	/* set socket received */
	(*socket) = *(int*)CMSG_DATA(cmsg);

	msg ("Process received socket %d, checking to send received signal...", (*socket));
	return axl_true;
}

/** 
 * @internal Function used to create an string that represents the
 * status of the BEEP session so the receiving process can reconstruct
 * the connection.
 */
char * turbulence_process_connection_status_string (axl_bool          handle_start_reply,
						    int               channel_num,
						    const char      * profile,
						    const char      * profile_content,
						    VortexEncoding    encoding,
						    const char      * serverName,
						    int               msg_no,
						    int               seq_no,
						    int               seq_no_expected,
						    int               ppath_id)
{
	return axl_strdup_printf ("n%d;-;%d;-;%s;-;%s;-;%d;-;%s;-;%d;-;%d;-;%d;-;%d",
				  handle_start_reply,
				  channel_num,
				  profile ? profile : "",
				  profile_content ? profile_content : "",
				  encoding,
				  serverName ? serverName : "",
				  ppath_id,
				  msg_no,
				  seq_no,
				  seq_no_expected);
}

void turbulence_process_send_connection_to_child (TurbulenceCtx    * ctx, 
						  TurbulenceChild  * child, 
						  VortexConnection * conn, 
						  axl_bool           handle_start_reply, 
						  int                channel_num,
						  const char       * profile, 
						  const char       * profile_content,
						  VortexEncoding     encoding, 
						  const char       * serverName, 
						  VortexFrame      * frame)
{
	VORTEX_SOCKET        client_socket;
	VortexChannel      * channel0    = vortex_connection_get_channel (conn, 0);
	TurbulencePPathDef * ppath       = turbulence_ppath_selected (conn);
	char               * conn_status;

	/* build connection status string */
	conn_status = turbulence_process_connection_status_string (handle_start_reply, 
								   channel_num,
								   profile,
								   profile_content,
								   encoding,
								   serverName,
								   vortex_frame_get_msgno (frame),
								   vortex_channel_get_next_seq_no (channel0),
								   vortex_channel_get_next_expected_seq_no (channel0),
								   turbulence_ppath_get_id (ppath));
	
	msg ("Sending connection to child already created, ancillary data ('%s') size: %d", conn_status, strlen (conn_status));
	/* unwatch the connection from the parent to avoid receiving
	   more content which now handled by the child and unregister
	   from connection manager */
	vortex_reader_unwatch_connection (CONN_CTX (conn), conn);

	/* socket that is know handled by the child process */
	client_socket = vortex_connection_get_socket (conn);

	/* send the socket descriptor to the child to avoid holding a
	   bucket in the parent */
	if (! turbulence_process_send_socket (client_socket, child, conn_status, strlen (conn_status))) {
		error ("Something failed while sending socket (%d) to child already created, error (code %d): %s",
		       client_socket, errno, vortex_errno_get_last_error ());
		/* release ancillary data */
		axl_free (conn_status);

		/* close connection */
		vortex_connection_shutdown (conn);
		return;
	}

	/* release ancillary data */
	axl_free (conn_status);
	
	/* terminate the connection */
	vortex_connection_set_close_socket (conn, axl_false);
	vortex_connection_shutdown (conn);

	return;
}

/** 
 * @internal Function used to implement common functions for a new
 * connection that is accepted into a child process.
 */
void __turbulence_process_common_new_connection (TurbulenceCtx      * ctx,
						 VortexConnection   * conn,
						 TurbulencePPathDef * def,
						 axl_bool             handle_start_reply,
						 int                  channel_num,
						 const char         * profile,
						 const char         * profile_content,
						 VortexEncoding       encoding,
						 const char         * serverName,
						 VortexFrame        * frame)
{
	VortexChannel * channel0;

	/* now notify profile path selected after dropping
	   priviledges */
	if (! turbulence_module_notify (ctx, TBC_PPATH_SELECTED_HANDLER, def, conn, NULL)) {
		/* close the connection */
		TBC_FAST_CLOSE (conn);

		/* check if clean start is activated to close the
		 * connection */
		CLEAN_START(ctx); /* check to terminate child if clean start is defined */
		return;
	}

	if (! vortex_connection_is_ok (conn, axl_false)) {
		error ("Connection id=%d is not working after notifying ppath selected handler..", vortex_connection_get_id (conn));
		vortex_connection_close (conn);
		return;
	}

	/* check if the connection was received during on connect
	   phase (vortex_listener_set_on_accept_handler) or because a
	   channel start */
	if (! handle_start_reply) {
		/* set the connection to still finish BEEP greetings
		   negotiation phase */
		vortex_connection_set_initial_accept (conn, axl_true);
	}

	/* now finish and register the connection */
	vortex_connection_set_close_socket (conn, axl_true);
	vortex_reader_watch_connection (TBC_VORTEX_CTX (ctx), conn);

	/* because the conn manager module has been initialized again,
	   register the connection handling by this process */
	turbulence_conn_mgr_register (ctx, conn);

	/* check to handle start reply message */
	msg ("Checking to handle start channel reply=%d at child=%d", handle_start_reply, getpid ());
	if (handle_start_reply) {
		/* handle start channel reply */
		if (! vortex_channel_0_handle_start_msg_reply (TBC_VORTEX_CTX (ctx), conn, channel_num,
							       profile, profile_content,
							       encoding, serverName, frame)) {
			error ("Channel start not accepted on child process, finising process=%d, closing conn id=%d..",
			       getpid (), vortex_connection_get_id (conn));

			/* wait here so the error message reaches the
			 * remote BEEP peer */
			channel0 = vortex_connection_get_channel (conn, 0);
			if (channel0 != NULL) 
				vortex_channel_block_until_replies_are_sent (channel0, 1000);
		} else {
			msg ("Channel start accepted on child..");
		}
	} /* end if */

	/* unref connection since it is registered */
	vortex_connection_unref (conn, "turbulence process, conn registered");

	return;
}

int __get_next_field (char * conn_status, int _iterator)
{
	int iterator = _iterator;

	while (conn_status[iterator]     != 0 && 
	       conn_status[iterator + 1] != 0 && 
	       conn_status[iterator + 2] != 0) {

		/* check if the separator was found (;-;) */
		if (conn_status [iterator] == ';' &&
		    conn_status [iterator + 1] == '-' &&
		    conn_status [iterator + 2] == ';') {
			conn_status[iterator] = 0;
			return iterator + 3;
		} /* end if */

		/* separator not found */
		iterator++;
	}

	return _iterator;
}

/** 
 * @internal Function that recovers the data to reconstruct a
 * connection state from the provided string.
 */
void     turbulence_process_connection_recover_status (char            * conn_status,
						       axl_bool        * handle_start_reply,
						       int             * channel_num,
						       const char     ** profile,
						       const char     ** profile_content,
						       VortexEncoding  * encoding,
						       const char     ** serverName,
						       int             * msg_no,
						       int             * seq_no,
						       int             * seq_no_expected,
						       int             * ppath_id)
{
	int iterator = 0;
	int next;

	/* parse ancillary data */
	conn_status[1]  = 0;
	(*handle_start_reply) = atoi (conn_status);

	/* get next position */
	iterator           = 4;
	next               = __get_next_field (conn_status, 4);
	(*channel_num) = atoi (conn_status + iterator); 

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*profile)         = conn_status + iterator;
	if (strlen (*profile) == 0)
		*profile = 0;

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*profile_content) = conn_status + iterator;
	if (strlen (*profile_content) == 0)
		*profile_content = 0;

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*encoding)        = atoi (conn_status + iterator);

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*serverName)      = conn_status + iterator;	
	if (strlen (*serverName) == 0)
		*serverName = 0;

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*ppath_id) = atoi (conn_status + iterator);

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*msg_no) = atoi (conn_status + iterator);

	/* get next position */
	iterator           = next;
	next               = __get_next_field (conn_status, iterator);
	(*seq_no) = atoi (conn_status + iterator);

	/* get next position */
	iterator           = next;
	next               = 0;
	(*seq_no_expected) = atoi (conn_status + iterator);

	return;
}

axl_bool turbulence_handle_connection_received (TurbulenceCtx      * ctx, 
						TurbulencePPathDef * ppath,
						VORTEX_SOCKET        socket, 
						char               * conn_status, 
						int                  size)
{
	axl_bool           handle_start_reply = axl_false;
	int                channel_num        = -1;
	const char       * profile            = NULL;
	const char       * profile_content    = NULL;
	const char       * serverName         = NULL;
	VortexEncoding     encoding           = EncodingNone;
	VortexConnection * conn               = NULL;
	int                msg_no             = -1;
	int                seq_no             = -1;
	int                seq_no_expected    = -1;
	int                ppath_id           = -1;
	VortexFrame      * frame              = NULL;
	VortexChannel    * channel0;


	/* call to recover data from string */
	turbulence_process_connection_recover_status (conn_status, 
						      &handle_start_reply,
						      &channel_num,
						      &profile, 
						      &profile_content,
						      &encoding,
						      &serverName,
						      &msg_no,
						      &seq_no,
						      &seq_no_expected,
						      &ppath_id);

	msg ("Received ancillary data: handle_start_reply=%d, channel_num=%d, profile=%s, profile_content=%s, encoding=%d, serverName=%s, msg_no=%d, seq_no=%d, ppath_id=%d",
	     handle_start_reply, channel_num, 
	     profile ? profile : "", 
	     profile_content ? profile_content : "", encoding, 
	     serverName ? serverName : "",
	     msg_no,
	     seq_no,
	     ppath_id);

	/* create a connection and register it on local vortex
	   reader */
	conn = vortex_connection_new_empty (TBC_VORTEX_CTX (ctx), socket, VortexRoleListener);
	msg ("New connection id=%d (%s:%s) accepted on child pid=%d", 
	     vortex_connection_get_id (conn), vortex_connection_get_host (conn), vortex_connection_get_port (conn), getpid ());

	/* set profile path state */
	__turbulence_ppath_set_state (ctx, conn, ppath_id, serverName);

	if (handle_start_reply) {
		/* build a fake frame to simulate the frame received from the
		   parent */
		frame = vortex_frame_create (TBC_VORTEX_CTX (ctx), 
					     VORTEX_FRAME_TYPE_MSG,
					     0, msg_no, axl_false, -1, 0, 0, NULL);
		/* update channel 0 status */
		channel0 = vortex_connection_get_channel (conn, 0);
		__vortex_channel_set_state (channel0, msg_no, seq_no, seq_no_expected, 0);
	}

	/* call to register */
	__turbulence_process_common_new_connection (ctx, conn, ppath,
						    handle_start_reply, channel_num,
						    profile, profile_content,
						    encoding, serverName,
						    frame);
	/* unref frame here */
	vortex_frame_unref (frame);
	return axl_true;
}

/** 
 * @internal Function called each time a notification from the parent
 * is received on the child.
 */
axl_bool turbulence_process_parent_notify (TurbulenceLoop * loop, 
					   TurbulenceCtx  * ctx,
					   int              descriptor, 
					   axlPointer       ptr, 
					   axlPointer       ptr2)
{
	int                socket = -1;
	TurbulenceChild  * child  = (TurbulenceChild *) ptr;
	char             * ancillary_data = NULL;
	int                size;
	
	msg ("Parent notification on control connection, read content");

	/* receive socket */
	if (! turbulence_process_receive_socket (&socket, child, &ancillary_data, &size)) {
		error ("Failed to received socket from parent..");
		return axl_false; /* close parent notification socket */
	}

	/* check content received */
	if (ancillary_data == NULL || strlen (ancillary_data) == 0 || socket <= 0) {
		error ("Ancillary data is null (%p) or empty or socket returned is not valid (%d)",
		       ancillary_data, socket);
		goto release_content;
	}

	msg ("Received socket %d, and ancillary_data[0]='%c'", socket, ancillary_data[0]);

	/* process commands received from the parent */
	if (ancillary_data[0] == 's') {
		/* close the connection received. This command signals
		   that the socket notified is already owned by the
		   current process (due to fork call) but the parent
		   still send us this socket to avoid having a file
		   descriptor used bucket. */
	} else if (ancillary_data[0] == 'n') {
		/* received notification if a new, unknown connection,
		   register it */
		msg ("Received new connection from parent, socket=%d", socket);
		turbulence_handle_connection_received (ctx, child->ppath, socket, ancillary_data + 1, size);
		socket = -1; /* avoid socket be closed */
	} else {
		msg ("Unknown command, socket received (%d), ancillary data: %s, size: %d", socket, ancillary_data, size);
	}

	/* release data received */
 release_content:
	axl_free (ancillary_data);
	vortex_close_socket (socket);

	return axl_true; /* don't close descriptor */
}

axl_bool __turbulence_process_release_parent_connections_foreach  (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2) 
{
	TurbulenceConnMgrState * state         = data; 
	TurbulenceChild        * parent        = user_data;
	TurbulenceCtx          * ctx           = parent->ctx;
	int                      client_socket = vortex_connection_get_socket (state->conn);
	VortexConnection       * child_conn    = user_data2;
	int                      ref_count;

	/* do handle connections already nullified */
	if (state->conn == NULL)
		return axl_false;

	/* remove installed handlers (channel added and channel removed) */
	vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_ADD_HANDLER, state->added_channel_id);
	vortex_connection_remove_handler (state->conn, CONNECTION_CHANNEL_REMOVE_HANDLER, state->removed_channel_id);

	/* remove previous on close (defined in the parent context no
	   matter if the connection is handled by the parent or the
	   current child) */
	vortex_connection_remove_on_close_full (state->conn, turbulence_conn_mgr_on_close, state);

	/* don't close/send the socket that the child must handle */
	if (vortex_connection_get_id (child_conn) == 
	    vortex_connection_get_id (state->conn)) {
		ref_count = vortex_connection_ref_count (state->conn);
		msg ("NOT Sending connection id=%d to parent (now handled by child) (ref count: %d, pointer: %p, role: %d)", 
		     vortex_connection_get_id (state->conn),
		     ref_count,
		     state->conn, vortex_connection_get_role (state->conn));

		/* reduce reference count */
		vortex_connection_unref (state->conn, "child process handled");

		state->conn = NULL;
		return axl_false;
	}

	/* send the socket descriptor to the parent to avoid holding a
	   bucket in the child (don't wait) */
	if (! turbulence_process_send_socket (client_socket, parent, "s", 1)) {
		error ("Failed to send socket to parent process, error was: %d (%s)", errno, vortex_errno_get_last_error ());
		return axl_false;
	} /* end if */

	/* get connection ref count */
	ref_count = vortex_connection_ref_count (state->conn);
	msg ("Connection id=%d sent to parent (ref count: %d, pointer: %p, role: %d)", 
	     vortex_connection_get_id (state->conn),
	     ref_count,
	     state->conn, vortex_connection_get_role (state->conn));

	/* terminate the connection */
	vortex_connection_set_close_socket (state->conn, axl_false);
	vortex_connection_shutdown (state->conn); 

	/* now unref the connection as much times as required leaving
	   the ref count equal to 1 so next calls to vortex
	   reinitialization, which reinitializes the vortex reader,
	   will release its reference */
	while (ref_count > 1)  {
		vortex_connection_unref (state->conn, "child process");
		ref_count--;
	}

	/* nullify state */
	state->conn = NULL;

	return axl_false;
}

/** 
 * @internal This function ensures that all connections that the
 * parent handles but the child must't are closed properly so the
 * child process only have access to connections associated to its
 * profile path.
 *
 * The idea is that the parent (main process) may have a number of
 * running connections serving certain profile paths...but when it is
 * required to create a child, a fork is done (unix) and all sockets
 * associated running at the parent, are available and the child so it
 * is required to release all this stuff.
 *
 * The function has two steps: first check all connections in the
 * connection manager hash closing all of them but skipping the
 * connection that must handle the child (that is, the connection that
 * triggered the fork) and the second step to initialize the
 * connection manager hash. Later the connection that must handle the
 * child is added to the newly initialized connection manager hash by
 * issuing a turbulence_conn_mgr_register.
 */
void __turbulence_process_release_parent_connections (TurbulenceCtx * ctx, TurbulenceChild * parent, VortexConnection * child_conn)
{
	/* clear the hash */
	axl_hash_foreach2 (ctx->conn_mgr_hash, __turbulence_process_release_parent_connections_foreach, parent, child_conn);

	/* reinit */
	turbulence_conn_mgr_init (ctx, axl_true);
	return;
}

/** 
 * @internal Allows to create a child process running listener connection
 * provided.
 */
void turbulence_process_create_child (TurbulenceCtx       * ctx, 
				      VortexConnection    * conn, 
				      TurbulencePPathDef  * def,
				      axl_bool              handle_start_reply,
				      int                   channel_num,
				      const char          * profile,
				      const char          * profile_content,
				      VortexEncoding        encoding,
				      const char          * serverName,
				      VortexFrame         * frame)
{
	int                pid;
	VortexCtx        * vortex_ctx;
	TurbulenceChild  * child;
	int                client_socket;
	VortexAsyncQueue * queue;
	char             * temp_dir;

	/* pipes to communicate logs from child to parent */
	int                general_log[2] = {-1, -1};
	int                error_log[2]   = {-1, -1};
	int                access_log[2]  = {-1, -1};
	int                vortex_log[2]  = {-1, -1};
	TurbulenceLoop   * control;
	const char       * ppath_name;

	/* check if we are main process (only main process can create
	 * childs, at least for now) */
	if (! ctx->is_main_process) {
		error ("Internal runtime error, child process %d is trying to create another subchild, closing conn-id=%d", 
		       ctx->pid, vortex_connection_get_id (conn));
		vortex_connection_shutdown (conn);
		return;
	} /* end if */

	/* get profile path name */
	ppath_name     = turbulence_ppath_get_name (def) ? turbulence_ppath_get_name (def) : "";

	msg2 ("Calling to create child process to handle profile path: %s..", ppath_name);
	vortex_mutex_lock (&ctx->child_process_mutex);
	msg2 ("LOCK acquired: calling to create child process to handle profile path: %s..", ppath_name);
	
	/* check if child associated to the given profile path is
	   defined and if reuse flag is enabled */
	child = turbulence_process_get_child_from_ppath (ctx, def, axl_false);
	if (def->reuse && child) {
		msg ("Found child process reuse flag and child already created (%p), sending connection id=%d, frame msgno=%d",
		     child, vortex_connection_get_id (conn), vortex_frame_get_msgno (frame));
		     
		/* reuse profile path */
		turbulence_process_send_connection_to_child (ctx, child, conn, 
							     handle_start_reply, channel_num,
							     profile, profile_content,
							     encoding, serverName, frame);
		vortex_mutex_unlock (&ctx->child_process_mutex);
		return;
	}
	if (child == NULL) 
		msg ("Creating a child process (first instance)");
	else
		msg ("Childs defined, but not reusing child processes (reuse=no flag)");

	if (turbulence_log_is_enabled (ctx)) {
		if (pipe (general_log) != 0)
			error ("unable to create pipe to transport general log, this will cause these logs to be lost");
		if (pipe (error_log) != 0)
			error ("unable to create pipe to transport error log, this will cause these logs to be lost");
		if (pipe (access_log) != 0)
			error ("unable to create pipe to transport access log, this will cause these logs to be lost");
		if (pipe (vortex_log) != 0)
			error ("unable to create pipe to transport vortex log, this will cause these logs to be lost");
	} /* end if */

	/* create control socket path */
	child        = axl_new (TurbulenceChild, 1);
	child->ppath = def;

	/* create socket path */
	child->socket_control_path = axl_strdup_printf ("%s%s%s%s%ld.tbc",
							turbulence_runtime_datadir (ctx),
							VORTEX_FILE_SEPARATOR,
							"turbulence",
							VORTEX_FILE_SEPARATOR,
							random ());
	/* check base dir exists */
	temp_dir = turbulence_base_dir (child->socket_control_path);
	if (temp_dir && ! vortex_support_file_test (temp_dir, FILE_EXISTS)) {
		/* base directory having child socket control do not exists */
		wrn ("run time directory %s do not exists, creating..", temp_dir);
		if (! turbulence_create_dir (temp_dir)) {
			error ("Unable to create directory to hold child socket connection, unable to create child process..");
			axl_free (temp_dir);
			vortex_mutex_unlock (&ctx->child_process_mutex);
			return;
		} /* end if */
	} /* end if */
	/* free temporal directory */
	axl_free (temp_dir);

	msg ("Creating socket_control_path = '%s'", child->socket_control_path);

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		msg ("PARENT: child process created pid=%d, selected-ppath=%s", pid, def->path_name ? def->path_name : "(empty)");
		/* unwatch the connection from the parent to avoid
		   receiving more content which now handled by the
		   child and unregister from connection manager */
		vortex_reader_unwatch_connection (CONN_CTX (conn), conn);

		/* update child pid and additional data */
		child->pid = pid;
		child->ctx = ctx;

		/* create child connection socket */
		if (! __turbulence_process_create_child_connection (child)) {
			vortex_mutex_unlock (&ctx->child_process_mutex);
			vortex_connection_shutdown (conn);
			turbulence_process_free_child_data (child);
			return;
		}

		/* socket that is know handled by the child process */
		client_socket = vortex_connection_get_socket (conn);

		/* send the socket descriptor to the child to avoid
		   holding a bucket in the parent */
		if (! turbulence_process_send_socket (client_socket, child, "s", 1)) {
			vortex_mutex_unlock (&ctx->child_process_mutex);
			vortex_connection_shutdown (conn);
			turbulence_process_free_child_data (child);
			return;
		}

		/* terminate the connection */
		vortex_connection_set_close_socket (conn, axl_false);
		vortex_connection_shutdown (conn);

		/* register pipes to receive child logs */
		if (turbulence_log_is_enabled (ctx)) {
			/* support for general-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_GENERAL, general_log[0]); /* register read end */
			vortex_close_socket (general_log[1]);                                      /* close write end */

			/* support for error-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_ERROR, error_log[0]); /* register read end */
			vortex_close_socket (error_log[1]);                                      /* close write end */

			/* support for access-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_ACCESS, access_log[0]); /* register read end */
			vortex_close_socket (access_log[1]);                                      /* close write end */

			/* support for vortex-log */
			turbulence_log_manager_register (ctx, LOG_REPORT_VORTEX, vortex_log[0]); /* register read end */
			vortex_close_socket (vortex_log[1]);                                      /* close write end */
		} /* end if */

		/* register the child process identifier */
		axl_hash_insert_full (ctx->child_process,
				      /* key and destroy func */
				      INT_TO_PTR (turbulence_ppath_get_id (def)), NULL,
				      /* data and destroy func */
				      child, (axlDestroyFunc) turbulence_process_free_child_data);
		vortex_mutex_unlock (&ctx->child_process_mutex);

		/* record child */
		msg ("PARENT=%d: Created child process pid=%d (childs: %d)", getpid (), pid, turbulence_process_child_count (ctx));
		return;
	} /* end if */
	msg ("CHILD: child started with process created pid=%d, selected-ppath=%s (ppath id: %d)", 
	     getpid (), def->path_name ? def->path_name : "(empty)", turbulence_ppath_get_id (def));

	/* do not log messages until turbulence_ctx_reinit finishes */
	child_ctx        = ctx;

	/* record profile path selected */
	ctx->child_ppath = def;

	/* reinit TurbulenceCtx */
	turbulence_ctx_reinit (ctx);

	/* now create child connection, connecting this way with the
	   parent */
	/* create child connection socket */
	child->ctx = ctx;
	msg ("CHILD: process creating control connection..");
	if (! __turbulence_process_create_parent_connection (child)) {
	        error ("Failed to create parent connection, finishing child process..");
		vortex_connection_shutdown (conn);
		turbulence_process_free_child_data (child);
		return;
	}

	/* release connections received from parent (including
	   sockets) */
	msg ("CHILD: calling to release all (parent) connections but conn-id=%d", 
	     vortex_connection_get_id (conn));
	__turbulence_process_release_parent_connections (ctx, child, conn);
	
	msg ("CHILD: all parent connections released, continue with child process preparation..");

	/* check if log is enabled to redirect content to parent */
	if (turbulence_log_is_enabled (ctx)) {
		/* configure new general log */
		turbulence_log_configure (ctx, LOG_REPORT_GENERAL, general_log[1]); /* configure write end */
		vortex_close_socket (general_log[0]);                               /* close read end */

		/* configure new error log */
		turbulence_log_configure (ctx, LOG_REPORT_ERROR, error_log[1]); /* configure write end */
		vortex_close_socket (error_log[0]);                             /* close read end */

		/* configure new access log */
		turbulence_log_configure (ctx, LOG_REPORT_ACCESS, access_log[1]); /* configure write end */
		vortex_close_socket (access_log[0]);                              /* close read end */

		/* configure new vortex log */
		turbulence_log_configure (ctx, LOG_REPORT_VORTEX, vortex_log[1]); /* configure write end */
		vortex_close_socket (vortex_log[0]);                              /* close read end */
	}

	/* cleanup log stuff used only by the parent process */
	turbulence_loop_close (ctx->log_manager, axl_false);
	ctx->log_manager = NULL;

	/* reconfigure signals */
	turbulence_signal_install (ctx, 
				   /* disable sigint */
				   axl_false, 
				   /* signal sighup */
				   axl_false,
				   /* enable sigchild */
				   axl_false,
				   turbulence_process_signal_received);

	msg ("CHILD: Created child process: %d", getpid ());

	/* reinit vortex ctx */
	vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);
	vortex_ctx_reinit (vortex_ctx);

	/* set finish handler that will help to finish child process
	   (or not) */
	ctx->child_wait = vortex_async_queue_new ();
	vortex_ctx_set_on_finish (vortex_ctx, turbulence_process_finished, child);

	/* (re)start vortex */
	if (! vortex_init_ctx (vortex_ctx)) {
		error ("failed to restart vortex engine after fork operation");
		exit (-1);
	} /* end if */

	/* (re)initialize here all modules */
	if (! turbulence_module_notify (ctx, TBC_INIT_HANDLER, NULL, NULL, NULL)) {
		/* close the connection */
		TBC_FAST_CLOSE (conn);

		CLEAN_START(ctx); /* check to terminate child if clean start is defined */
	}

	/* check here to change root path, in the case it is defined
	 * now we still have priviledges */
	turbulence_ppath_change_root (ctx, def);

	/* check here for setuid support */
	turbulence_ppath_change_user_id (ctx, def);

	/* perfom common tasks for new connection acceptance */
	msg ("CHILD: handling new connection (first) at child connection process, handle_start_reply=%d, frame=%p",
	     handle_start_reply, frame);
	__turbulence_process_common_new_connection (ctx, conn, def, 
						    handle_start_reply, channel_num, 
						    profile, profile_content,
						    encoding, serverName,
						    frame);
	/* create parent control loop */
	msg ("CHILD: starting child loop to watch for sockets from parent");
	control = turbulence_loop_create (ctx);
	turbulence_loop_watch_descriptor (control, child->child_connection, 
					  turbulence_process_parent_notify, child, NULL);
	msg ("CHILD: started socket watch on (%d)", child->child_connection);
	
	msg ("CHILD: process created OK...wait for requests");
	queue = ctx->child_wait;

	/* flag that the server started ok */
	ctx->started = axl_true;

	vortex_async_queue_pop (ctx->child_wait);
	msg ("finishing process...");

	/* call to uninstall finish watcher */
	vortex_ctx_set_on_finish (vortex_ctx, NULL, NULL);

	/* terminate parent loop */
	turbulence_loop_close (control, axl_true);
	
	/* release frame received */
	vortex_frame_unref (frame);

	/* release child descriptor */
	turbulence_process_free_child_data (child);

	/* terminate turbulence execution */
	turbulence_exit (ctx, axl_false, axl_false);

	/* free context (the very last operation) */
	vortex_ctx_free (vortex_ctx);

	/* unref the queue here to avoid vortex finish handler to
	   access to the queue */
	vortex_async_queue_unref (queue);

	/* finish process */
	turbulence_ctx_free (ctx);
	exit (0);
	
	return;
}

#if defined(DEFINE_KILL_PROTO)
int kill (int pid, int signal);
#endif

axl_bool __terminate_child (axlPointer key, axlPointer data, axlPointer user_data)
{
	TurbulenceCtx   * ctx   = user_data;
	TurbulenceChild * child = data;

	/* send term signal */
	if (kill (child->pid, SIGTERM) != 0)
		error ("failed to kill child (%d) error was: %d:%s",
		       child->pid, errno, vortex_errno_get_last_error ());
	return axl_false; /* keep on iterating */
}

/** 
 * @internal Function that allows to check and kill childs started by
 * turbulence acording to user configuration.
 *
 * @param ctx The context where the child stop operation will take
 * place.
 */ 
void turbulence_process_kill_childs  (TurbulenceCtx * ctx)
{
	axlDoc  * doc;
	axlNode * node;
	int       pid;
	int       status;
	int       childs;

	/* get user doc */
	doc  = turbulence_config_get (ctx);
	node = axl_doc_get (ctx->config, "/turbulence/global-settings/kill-childs-on-exit");
	if (node == NULL) {
		error ("Unable to find kill-childs-on-exit node, doing nothing..");
		return;
	}

	/* check if we have to kill childs */
	if (! HAS_ATTR_VALUE (node, "value", "yes")) {
		error ("leaving childs running (kill-childs-on-exit not enabled)..");
		return;
	} /* end if */

	/* disable signal handling because we are the parent and we
	   are killing childs (we know childs are stopping). The
	   following is to avoid races with
	   turbulence_signal_received for SIGCHLD */
	signal (SIGCHLD, NULL);

	/* send a kill operation to all childs */
	childs = axl_hash_items (ctx->child_process);
	if (childs > 0) {
		/* lock child to get first element and remove it */
		vortex_mutex_lock (&ctx->child_process_mutex);

		/* notify all childs they will be closed */
		axl_hash_foreach (ctx->child_process, __terminate_child, ctx);

		/* unlock the list during the kill and wait */
		vortex_mutex_unlock (&ctx->child_process_mutex);

		while (childs > 0) {
			/* wait childs to finish */
			msg ("waiting childs (%d) to finish...", childs);
			status = 0;
			pid = wait (&status);
			msg ("...child %d finish, exit status: %d", pid, status);
			
			childs--;
		} /* end while */
	} /* end while */


	return;
}

/** 
 * @brief Allows to return the number of child processes  created. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @return Number of child process created or -1 it if fails.
 */
int      turbulence_process_child_count  (TurbulenceCtx * ctx)
{
	int count;

	/* check context received */
	if (ctx == NULL)
		return -1;

	vortex_mutex_lock (&ctx->child_process_mutex);
	count = axl_hash_items (ctx->child_process);
	vortex_mutex_unlock (&ctx->child_process_mutex);
	msg ("child process count: %d..", count);
	return count;
}

/** 
 * @brief Allows to check if the provided process Id belongs to a
 * child process currently running.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param pid The process identifier.
 *
 * @return axl_true if the process exists, otherwise axl_false is
 * returned.
 */
axl_bool turbulence_process_child_exits  (TurbulenceCtx * ctx, int pid)
{
	/* check context received */
	if (ctx == NULL || pid < 0)
		return axl_false;

	return (turbulence_process_find_pid_from_ppath_id (ctx, pid) != -1);
}

/** 
 * @internal Function used by
 * turbulence_process_find_pid_from_ppath_id
 */
axl_bool __find_ppath_id (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2) 
{
	int               ppath_id   = PTR_TO_INT (key);
	int             * pid        = (int *) user_data;
	int             * _ppath_id  = (int *) user_data2;
	TurbulenceChild * child      = data;
	
	if (child->pid == (*pid)) {
		/* child found, update pid to have ppath_id */
		(*_ppath_id) = ppath_id;
		return axl_true; /* found key, stop foreach */
	}
	return axl_false; /* child not found, keep foreach looping */
}

/** 
 * @brief The function returns the profile path associated to the pid
 * child provided. The pid represents a child created by turbulence
 * due to a profile path selected.
 *
 * @param ctx The turbulence context where the ppath identifier will be looked up.
 * @param pid The child pid to be used during the search.
 *
 * @return The function returns -1 in the case of failure or the
 * ppath_id associated to the child process.
 */
int      turbulence_process_find_pid_from_ppath_id (TurbulenceCtx * ctx, int pid)
{
	int ppath_id = -1;
	
	vortex_mutex_lock (&ctx->child_process_mutex);
	axl_hash_foreach2 (ctx->child_process, __find_ppath_id, &pid, &ppath_id);
	vortex_mutex_unlock (&ctx->child_process_mutex);

	/* check that the pid was found */
	return ppath_id;
	
}

/** 
 * @internal Function used by
 * turbulence_process_get_child_from_ppath
 */
axl_bool __find_ppath (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2) 
{
	int                   ppath_id   = PTR_TO_INT (key);
	TurbulencePPathDef  * ppath      = user_data;
	TurbulenceChild     * child      = data;
	TurbulenceChild    ** result     = user_data2;
	
	if (ppath_id == turbulence_ppath_get_id (ppath)) {
		/* found child associated, updating reference and
		   signaling to stop earch */
		(*result) = child;
		return axl_true; /* found key, stop foreach */
	} /* end if */
	return axl_false; /* child not found, keep foreach looping */
}

/** 
 * @internal Allows to get the child associated to the profile path
 * definition.
 *
 * @param ctx The context where the lookup will be implemented.
 *
 * @param def The turbulence profile path definition to use to select
 * the child process associted.
 *
 * @param acquire_mutex Acquire mutex to access child process hash.
 *
 * @return A reference to the child process or NULL it if fails.
 */
TurbulenceChild * turbulence_process_get_child_from_ppath (TurbulenceCtx * ctx, 
							   TurbulencePPathDef * def,
							   axl_bool             acquire_mutex)
{
	TurbulenceChild * result = NULL;
	
	if (acquire_mutex)
		vortex_mutex_lock (&ctx->child_process_mutex);
	axl_hash_foreach2 (ctx->child_process, __find_ppath, def, &result);
	if (acquire_mutex)
		vortex_mutex_unlock (&ctx->child_process_mutex);

	/* check that the pid was found */
	return result;
}

/** 
 * @internal Function used to cleanup the process module.
 */
void turbulence_process_cleanup      (TurbulenceCtx * ctx)
{
	vortex_mutex_destroy (&ctx->child_process_mutex);
	axl_hash_free (ctx->child_process);
	return;
			      
}

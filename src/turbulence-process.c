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


/** 
 * @internal Path to the turbulence binary path that used to load current
 * instance.
 */
const char * turbulence_bin_path         = NULL;

/** 
 * @internal Optional command prefix used to create child process in
 * turbulence.
 */
const char * turbulence_child_cmd_prefix = NULL;

extern axl_bool __turbulence_module_no_unmap;

/** 
 * @internal Macro used to block sigchild and lock the mutex
 * associated to child processes.
 */
#define TBC_PROCESS_LOCK_CHILD() do {                  \
	turbulence_signal_block (ctx, SIGCHLD);        \
	vortex_mutex_lock (&ctx->child_process_mutex); \
} while (0)

/** 
 * @internal Macro used to block sigchild and lock the mutex
 * associated to child processes.
 */
#define TBC_PROCESS_UNLOCK_CHILD() do {                  \
	vortex_mutex_unlock (&ctx->child_process_mutex); \
	turbulence_signal_unblock (ctx, SIGCHLD);        \
} while (0)

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


axlPointer __turbulence_process_finished (TurbulenceCtx * ctx)
{
	VortexCtx        * vortex_ctx = ctx->vortex_ctx;
	int                delay      = 300000; /* 300ms */
	int                tries      = 30; 

	while (tries > 0) {

		/* check if the conn manager has connections watched
		 * at this time */
		vortex_mutex_lock (&ctx->conn_mgr_mutex);
		if (axl_hash_items (ctx->conn_mgr_hash) > 0) {
			msg ("CHILD: cancelled child process termination because new connections are now handled, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d",
			      tries, delay, 
			      vortex_reader_connections_watched (vortex_ctx), 
			      axl_hash_items (ctx->conn_mgr_hash));
			vortex_mutex_unlock (&ctx->conn_mgr_mutex);
			return NULL;
		} /* end if */
		vortex_mutex_unlock (&ctx->conn_mgr_mutex);
		
		/* finish current thread if turbulence is existing */
		if (ctx->is_exiting) {
			msg ("CHILD: Found turbulence existing, finishing child process termination thread signaled due to vortex reader stop");
			return NULL;
		}

		if (tries < 10) {
			/* reduce tries, wait */
			msg ("CHILD: delay child process termination to ensure the parent has no pending connections, tries=%d, delay=%d, reader connections=%d, tbc conn mgr=%d, is-existing=%d",
			     tries, delay,
			     vortex_reader_connections_watched (vortex_ctx), 
			     axl_hash_items (ctx->conn_mgr_hash),
			     ctx->is_exiting);
		} /* end if */

		tries--;
		turbulence_sleep (ctx, delay);
	}

	/* unlock waiting child */
	msg ("CHILD: calling to unlock due to vortex reader stoped (no more connections to be watched): %p (vortex refs: %d)..",
	     ctx, vortex_ctx_ref_count (ctx->vortex_ctx));
	/* unlock the current listener */
	vortex_listener_unlock (TBC_VORTEX_CTX (ctx));
	return NULL;
}


void turbulence_process_check_for_finish (TurbulenceCtx * ctx)
{
	VortexThread                thread;

	/* do not implement any notification if the turbulence process
	   is itself finishing */
	if (ctx->is_exiting)
		return;

	/* check if the child process was configured with a reuse
	   flag, if not, notify exist right now */
	if (! ctx->child->ppath->reuse) {
		/* so it is a child process without reuse flag
		   activated, exit now */
		vortex_listener_unlock (TBC_VORTEX_CTX (ctx));
		return;
	}

	/* reached this point, the child process has reuse = yes which
	   means we have to ensure we don't miss any connection which
	   is already accepted on the parent but still not reached the
	   child: the following creates a thread to unlock the vortex
	   reader, so he can listen for new registration while we do
	   the wait code for new connections... */

	/* create data to be notified */
	msg ("CHILD: initiated child termination (checking for rogue connections..");
	if (! vortex_thread_create (&thread,
				    (VortexThreadFunc) __turbulence_process_finished,
				    ctx, VORTEX_THREAD_CONF_DETACHED, VORTEX_THREAD_CONF_END)) {
		error ("Failed to create thread to watch reader state and notify process termination (code %d) %s",
		       errno, vortex_errno_get_last_error ());
	}
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
	TurbulenceCtx * ctx = child->ctx;

	msg ("CHILD: process creating control connection: %s..", child->socket_control_path);
	child->child_connection = __turbulence_process_local_unix_fd (child->socket_control_path, axl_false, child->ctx);
	msg ("CHILD: child control connection status: %d", child->child_connection);
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
	int                * int_ptr;
	char                 ccmsg[CMSG_SPACE(sizeof(socket))];

	struct cmsghdr     * cmsg;
	struct iovec         vec; 

	/* send at least one byte */
	const char         * str = ancillary_data ? ancillary_data : "#"; 
	int                  rv;
	TurbulenceCtx      * ctx = child->ctx;

	/* clear structures */
	memset (&msg, 0, sizeof (struct msghdr));

	/* configure destination */
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
	int_ptr                = (int*)CMSG_DATA(cmsg);
	(*int_ptr)             = socket;

	msg.msg_controllen     = cmsg->cmsg_len;
	msg.msg_flags = 0;
	
	rv = (sendmsg (child->child_connection, &msg, 0) != -1);
	if (rv)  {
		msg ("Socket %d sent to child via %d, closing (status: %d)..", 
		     socket, child->child_connection, rv);
		/* close the socket  */
		vortex_close_socket (socket); 
	} else {
		error ("Failed to send socket, error code %d, textual was: %s", rv, vortex_errno_get_error (errno));
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
axl_bool turbulence_process_receive_socket (VORTEX_SOCKET    * _socket, 
					    TurbulenceChild  * child, 
					    char            ** ancillary_data, 
					    int              * size)
{
	struct msghdr    msg;
	struct iovec     iov;
	char             buf[1024];
	int              status;
	char             ccmsg[CMSG_SPACE(sizeof(int))];
	struct           cmsghdr *cmsg;
	TurbulenceCtx  * ctx;

	/* variables for error reporting */
	VORTEX_SOCKET    temp;
	int              soft_limit, hard_limit;

	int            * int_ptr;
	
	v_return_val_if_fail (_socket && child, axl_false);

	/* get context reference */
	ctx = child->ctx;
	
	iov.iov_base = buf;
	iov.iov_len = 8192;

	memset (&msg, 0, sizeof (struct msghdr));	
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
		(*_socket) = -1;
		return axl_false;
	} /* end if */

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		/* check first if we have support to create more sockets */
		temp = socket (AF_INET, SOCK_STREAM, 0);
		if (temp == VORTEX_INVALID_SOCKET) {
			/* we have received our socket limit */
			vortex_conf_get (TBC_VORTEX_CTX(ctx), VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
			vortex_conf_get (TBC_VORTEX_CTX(ctx), VORTEX_HARD_SOCK_LIMIT, &hard_limit);
		
			error ("Unable to receive socket from parent, droping socket connection, reached process limit: soft-limit=%d, hard-limit=%d\n",
			       soft_limit, hard_limit);
		} else {
			error ("Received empty control message from parent (status: %d), unable to receive socket (code %d): %s",
			       status, errno, vortex_errno_get_last_error ());
		} /* end if */

		(*_socket) = -1;
		if (ancillary_data)
			(*ancillary_data) = NULL;
		if (size)
			(*size) = 0;
		return axl_false;
	}

	if (!cmsg->cmsg_type == SCM_RIGHTS) {
		error ("Unexpected control message of unknown type %d, failed to receive socket", 
		       cmsg->cmsg_type);
		(*_socket) = -1;
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
	int_ptr    = (int *) CMSG_DATA(cmsg);
	(*_socket) = (*int_ptr);

	msg ("Process received socket %d, checking to send received signal...", (*_socket));
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

	/* socket that is know handled by the child process */
	client_socket = vortex_connection_get_socket (conn);
	vortex_connection_set_close_socket (conn, axl_false);

	/* unwatch the connection from the parent to avoid receiving
	   more content which now handled by the child and unregister
	   from connection manager */
	vortex_reader_unwatch_connection (CONN_CTX (conn), conn);

	/* send the socket descriptor to the child to avoid holding a
	   bucket in the parent */
	if (! turbulence_process_send_socket (client_socket, child, conn_status, strlen (conn_status))) {
		error ("PARENT: Something failed while sending socket (%d) to child pid %d already created, error (code %d): %s",
		       client_socket, child->pid, errno, vortex_errno_get_last_error ());

		/* release ancillary data */
		axl_free (conn_status);

		/* close connection */
		vortex_connection_shutdown (conn);
		return;
	}

	/* release ancillary data */
	axl_free (conn_status);
	
	/* terminate the connection */
	vortex_connection_shutdown (conn);

	return;
}

/** 
 * @internal Function used to implement common functions for a new
 * connection that is accepted into a child process.
 */
axl_bool __turbulence_process_common_new_connection (TurbulenceCtx      * ctx,
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
	axl_bool        result = axl_true;

	/* now notify profile path selected after dropping
	   priviledges */
	if (! turbulence_module_notify (ctx, TBC_PPATH_SELECTED_HANDLER, def, conn, NULL)) {
		/* close the connection */
		TBC_FAST_CLOSE (conn);

		/* check if clean start is activated to close the
		 * connection */
		CLEAN_START(ctx); /* check to terminate child if clean start is defined */
		return axl_false;
	}

	if (! vortex_connection_is_ok (conn, axl_false)) {
		error ("Connection id=%d is not working after notifying ppath selected handler..", vortex_connection_get_id (conn));
		vortex_connection_close (conn);
		return axl_false;
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
	msg ("Checking to handle start channel=%s serverName=%s reply=%d at child=%d", profile, serverName ? serverName : "", handle_start_reply, getpid ());
	if (handle_start_reply) {
		/* handle start channel reply */
		if (! vortex_channel_0_handle_start_msg_reply (TBC_VORTEX_CTX (ctx), conn, channel_num,
							       profile, profile_content,
							       encoding, serverName, frame)) {
			error ("Channel start not accepted on child=%d process, closing conn id=%d..sending error reply",
			       vortex_getpid (), vortex_connection_get_id (conn));

			/* wait here so the error message reaches the
			 * remote BEEP peer */
			channel0 = vortex_connection_get_channel (conn, 0);
			if (channel0 != NULL) 
				vortex_channel_block_until_replies_are_sent (channel0, 1000);

			/* because this connection is being handled at
			 * the child and the child did not accepted
			 * it, shutdown to force its removal */
			if (turbulence_config_is_attr_positive (
				    ctx, 
				    TBC_CONFIG_PATH ("/turbulence/global-settings/close-conn-on-start-failure"), "value")) {
				/* close connection */
				error ("   shutting down connection id=%d (refs: %d) because it was not accepted on child process due to start handle negative reply", 
				       vortex_connection_get_id (conn), vortex_connection_ref_count (conn));
				turbulence_conn_mgr_unregister (ctx, conn);
				vortex_connection_shutdown (conn); 
				result = axl_false; 
			}
		} else {
			msg ("Channel start accepted on child profile=%s, serverName=%s accepted on child", profile, serverName ? serverName : "");
		}
	} /* end if */

	/* unref connection since it is registered */
	vortex_connection_unref (conn, "turbulence process, conn registered");

	return result;
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

VortexConnection * __turbulence_process_handle_connection_received (TurbulenceCtx      * ctx, 
								    TurbulencePPathDef * ppath,
								    VORTEX_SOCKET        socket, 
								    char               * conn_status)
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
	if (! __turbulence_process_common_new_connection (ctx, conn, ppath,
							  handle_start_reply, channel_num,
							  profile, profile_content,
							  encoding, serverName, frame)) {
		/* nullify conn on error */
		conn = NULL;
	}
	    
	/* unref frame here */
	vortex_frame_unref (frame);
	return conn;
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
	int                socket         = -1;
	TurbulenceChild  * child          = (TurbulenceChild *) ptr;
	char             * ancillary_data = NULL;
	const char       * label          = ctx->child ? "CHILD" : "PARENT";
	
	msg ("%s: notification on control connection, read content", label);

	/* receive socket */
	if (! turbulence_process_receive_socket (&socket, child, &ancillary_data, NULL)) {
		error ("%s: Failed to received socket..", label);
		return axl_false; /* close parent notification socket */
	}

	/* check content received */
	if (ancillary_data == NULL || strlen (ancillary_data) == 0 || socket <= 0) {
		error ("%s: Ancillary data is null (%p) or empty or socket returned is not valid (%d)",
		       label, ancillary_data, socket);
		goto release_content;
	}

	msg ("%s: Received socket %d, and ancillary_data[0]='%c'", 
	     label, socket, ancillary_data[0]);

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
		msg ("%s: Received new connection from parent, socket=%d", 
		     label, socket);
		__turbulence_process_handle_connection_received (ctx, child->ppath, socket, ancillary_data + 1);
		socket = -1; /* avoid socket be closed */
	} else {
		msg ("%s: Unknown command, socket received (%d), ancillary data: %s", 
		     label, socket, ancillary_data);
	}

	/* release data received */
 release_content:
	axl_free (ancillary_data);
	vortex_close_socket (socket);

	return axl_true; /* don't close descriptor */
}

void __turbulence_process_release_parent_connections_foreach  (VortexConnection * conn, axlPointer user_data, axlPointer user_data2, axlPointer user_data3) 
{
	TurbulenceCtx          * ctx           = user_data;
	VortexConnection       * child_conn    = user_data2;
	int                      client_socket = vortex_connection_get_socket (conn);

	/* don't close/send the socket that the child must handle */
	if (vortex_connection_get_id (child_conn) == 
	    vortex_connection_get_id (conn)) {
		msg ("CHILD: NOT Sending connection id=%d (socket: %d) to parent (now handled by child) (pointer: %p, role: %d)", 
		     vortex_connection_get_id (conn), client_socket,
		     conn, vortex_connection_get_role (conn));
		return;
	} /* end if */

	/* setting close on exec flag for all sockets the child will not handle */
	msg ("CHILD: Setting close on exec on socket: %d (conn id: %d, role: %d)", client_socket, 
	     vortex_connection_get_id (conn), vortex_connection_get_role (conn));
	fcntl (client_socket, F_SETFD, fcntl(client_socket, F_GETFD) | FD_CLOEXEC);

	return;
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
int __turbulence_process_release_parent_connections (TurbulenceCtx    * ctx, 
						     VortexConnection * child_conn, 
						     TurbulenceChild  * child)
{
	int                client_socket;
	VortexConnection * conn; 

	/* set close on exec flag for all sockets but the socket the
	 * child will handle */
	vortex_reader_foreach_offline (ctx->vortex_ctx, __turbulence_process_release_parent_connections_foreach, 
				       ctx, child_conn, NULL);

	/* release temporal listener created by the parent for
	 * master<->child link  */
	conn          = child->conn_mgr;
	client_socket = vortex_connection_get_socket (conn);
	msg ("CHILD: Setting close on exec on socket: %d (conn id: %d, role: %d)", client_socket, 
	     vortex_connection_get_id (conn), vortex_connection_get_role (conn));
	     fcntl (client_socket, F_SETFD, fcntl(client_socket, F_GETFD) | FD_CLOEXEC); 
	return 0;
}

void __turbulence_process_prepare_logging (TurbulenceCtx * ctx, axl_bool is_parent, int * general_log, int * error_log, int * access_log, int * vortex_log)
{
	/* check if log is enabled or not */
	if (! turbulence_log_is_enabled (ctx))
		return;

	if (is_parent) {

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

		return;
	} /* end if */

	return;
}

/** 
 * @internal Allows to check child limit (global and profile path
 * limits).
 *
 * @param ctx The context where the limit is being checked.
 *
 * @param conn The connection that triggered the child creation
 * process.
 *
 * @param def The profile path def selected for this connection.
 *
 * @return axl_true in the case the limit was reched and the
 * connection was closed, otherwise axl_false is returned.
 */
axl_bool turbulence_process_check_child_limit (TurbulenceCtx      * ctx,
					       VortexConnection   * conn,
					       TurbulencePPathDef * def)
{
	int                global_child_limit;

	/* check limits here before continue */
	global_child_limit = turbulence_config_get_number (ctx, "/turbulence/global-settings/global-child-limit", "value");

	/* check for default or undefined */
	if (global_child_limit == -3 || global_child_limit == -1)
		global_child_limit = 100;

	msg ("Checking global child limit: %d before creating process.", global_child_limit);

	if (axl_hash_items (ctx->child_process) >= global_child_limit) {
		error ("Child limit reached (%d), unable to accept connection on child process, closing conn-id=%d", 
		       global_child_limit, vortex_connection_get_id (conn));

		vortex_connection_shutdown (conn);
		return axl_true; /* limit reached */
	} /* end if */	

	/* now check profile path limits */
	if (def && def->child_limit > 0) {
		/* check limits */
		if (def->childs_running >= def->child_limit) {
			error ("Profile path child limit reached (%d), unable to accept connection on child process, closing conn-id=%d", 
			       def->child_limit, vortex_connection_get_id (conn));
			
			vortex_connection_shutdown (conn);
			return axl_true; /* limit reached */
		} /* end if */
	} /* end if */

	return axl_false; /* limit NOT reached */
}

/** 
 * @internal Function used to send child init string to the child
 * process.
 */
axl_bool __turbulence_process_send_child_init_string (TurbulenceCtx       * ctx,
						      TurbulenceChild     * child,
						      VortexConnection    * conn,
						      TurbulencePPathDef  * def,
						      axl_bool              handle_start_reply,
						      int                   channel_num,
						      const char          * profile,
						      const char          * profile_content,
						      VortexEncoding        encoding,
						      char                * serverName,
						      VortexFrame         * frame,
						      int                 * general_log,
						      int                 * error_log,
						      int                 * access_log,
						      int                 * vortex_log)
{
	VortexChannel * channel0;
	char          * conn_status;
	char          * child_init_string;
	int             length;
	int             written;
	char          * aux;
	int             tries = 0;

	/* build connection status string */
	channel0    = vortex_connection_get_channel (conn, 0);
	conn_status = turbulence_process_connection_status_string (handle_start_reply, 
								   channel_num,
								   profile,
								   profile_content,
								   encoding,
								   serverName,
								   vortex_frame_get_msgno (frame),
								   vortex_channel_get_next_seq_no (channel0),
								   vortex_channel_get_next_expected_seq_no (channel0),
								   turbulence_ppath_get_id (def));
	if (conn_status == NULL) {
		error ("PARENT: failled to create child, unable to allocate conn status string");
		return axl_false;
	}

	msg ("PARENT: create connection status to handle at child: %s, handle_start_reply=%d", conn_status, handle_start_reply);

	/* prepare child init string: 
	 *
	 * 0) conn socket : the connection socket that will handle the child 
	 * 1) general_log[0] : read end for general log 
	 * 2) general_log[1] : write end for general log 
	 * 3) error_log[0] : read end for error log 
	 * 4) error_log[1] : write end for error log 
	 * 5) access_log[0] : read end for access log 
	 * 6) access_log[1] : write end for access log 
	 * 7) vortex_log[0] : read end for vortex log 
	 * 8) vortex_log[1] : write end for vortex log 
	 * 9) child->socket_control_path : path to the socket_control_path
	 * 10) ppath_id : profile path identification to be used on child (the profile path activated for this child)
	 * 11) conn_status : connection status description to recover it at the child
	 * 12) conn_mgr_host : host where the connection mgr is locatd (BEEP master<->child link)
	 * 13) conn_mgr_port : port where the connection mgr is locatd (BEEP master<->child link)
	 * POSITION INDEX:                       0    1    2    3    4    5    6    7    8    9   10   11   12   13*/
 	child_init_string = axl_strdup_printf ("%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%d;_;%s;_;%d;_;%s;_;%s;_;%s",
					       /* 0  */ vortex_connection_get_socket (conn),
					       /* 1  */ general_log[0],
					       /* 2  */ general_log[1],
					       /* 3  */ error_log[0],
					       /* 4  */ error_log[1],
					       /* 5  */ access_log[0],
					       /* 6  */ access_log[1],
					       /* 7  */ vortex_log[0],
					       /* 8  */ vortex_log[1],
					       /* 9  */ child->socket_control_path,
					       /* 10 */ turbulence_ppath_get_id (def),
					       /* 11 */ conn_status,
					       /* 12 */ vortex_connection_get_local_addr (child->conn_mgr),
					       /* 13 */ vortex_connection_get_local_port (child->conn_mgr));
	axl_free (conn_status);
	if (child_init_string == NULL) {
		error ("PARENT: failled to create child, unable to allocate memory for child init string");
		return axl_false;
	} /* end if */

	/* get child init string length */
	length = strlen (child_init_string);

	/* first send bytes to read */
	aux     = axl_strdup_printf ("%d\n", length);
	length  = strlen (aux);

	while (tries < 100) {
		written = write (child->child_connection, aux, length);
		if (written == length)
			break;
		tries++;
		if (errno == 107) {
			wrn ("PARENT: child still not ready, waiting 10ms (socket: %d), reconnecting..", child->child_connection);
			turbulence_sleep (ctx, 10000);
			vortex_close_socket (child->child_connection);
			if (! __turbulence_process_create_child_connection (child)) {
				error ("PARENT: error after reconnecting to child process, errno: %d:%s", errno, vortex_errno_get_last_error ());
				break;
			} /* end if */
		}
	} /* end while */
	axl_free (aux);

	/* check errors */
	if (written != length) {
		error ("PARENT: failed to send init child string, expected to write %d bytes but %d were written, errno: %d:%s",
		       length, written, errno, vortex_errno_get_last_error ());
		return axl_false;
	}

	/* get child init string length (again) */
	length = strlen (child_init_string);
	msg ("PARENT: sent init child string length %d, now send content..", length);

	/* send content */
	written = write (child->child_connection, child_init_string, length);
	axl_free (child_init_string);

	if (length != written) {
		error ("PARENT: failed to send init child string, expected to write %d bytes but %d were written",
		       length, written);
		return axl_false;
	}
	
	msg ("PARENT: child init string sent to child");
	return axl_true;
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
				      char                * serverName,
				      VortexFrame         * frame)
{
	int                pid;
	TurbulenceChild  * child;
	int                client_socket;
	
	/* pipes to communicate logs from child to parent */
	int                general_log[2] = {-1, -1};
	int                error_log[2]   = {-1, -1};
	int                access_log[2]  = {-1, -1};
	int                vortex_log[2]  = {-1, -1};
	const char       * ppath_name;
	int                error_code;
	char            ** cmds;
	int                iterator = 0;

	/* check if we are main process (only main process can create
	 * childs, at least for now) */
	if (ctx->child) {
		error ("Internal runtime error, child process %d is trying to create another subchild, closing conn-id=%d", 
		       ctx->pid, vortex_connection_get_id (conn));
		vortex_connection_shutdown (conn);
		return;
	} /* end if */

	/* get profile path name */
	ppath_name     = turbulence_ppath_get_name (def) ? turbulence_ppath_get_name (def) : "";

	msg2 ("Calling to create child process to handle profile path: %s..", ppath_name);

	TBC_PROCESS_LOCK_CHILD ();

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
		TBC_PROCESS_UNLOCK_CHILD ();
		return;
	}

	/* check limits here before continue */
	if (turbulence_process_check_child_limit (ctx, conn, def)) {
		/* unlock before shutting down connection */
		TBC_PROCESS_UNLOCK_CHILD ();
		return;
	}

	/* show some logs about what will happen */
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
	child        = turbulence_child_new (ctx, def);
	if (child == NULL) {
		/* unlock child process mutex */
		TBC_PROCESS_UNLOCK_CHILD ();

		/* close connection that would handle child process */
		vortex_connection_shutdown (conn);
		return;
	} /* end if */

	/* unregister from connection manager */
	msg ("PARENT: created temporal listener to prepare child management connection id=%d (socket: %d): %p (refs: %d)", 
	     vortex_connection_get_id (child->conn_mgr), vortex_connection_get_socket (child->conn_mgr),
	     child->conn_mgr, vortex_connection_ref_count (child->conn_mgr));

	/* call to fork */
	pid = fork ();
	if (pid != 0) {
		msg ("PARENT: child process created pid=%d, selected-ppath=%s", pid, def->path_name ? def->path_name : "(empty)");
		/* unwatch the connection from the parent to avoid
		   receiving more content which now handled by the
		   child */
		vortex_reader_unwatch_connection (CONN_CTX (conn), conn);

		/* update child pid and additional data */
		child->pid = pid;
		child->ctx = ctx;

		/* create child connection socket */
		if (! __turbulence_process_create_child_connection (child)) {
			error ("Unable to create child process connection to pass sockets for pid=%", pid);
		
			TBC_PROCESS_UNLOCK_CHILD ();

			vortex_connection_shutdown (conn);
			turbulence_child_unref (child);
			return;
		}

		/* send child init string through the control socket */
		if (! __turbulence_process_send_child_init_string (ctx, child, conn, def, 
								   handle_start_reply, channel_num, 
								   profile, profile_content, 
								   encoding, serverName, frame,
								   general_log, error_log, access_log, vortex_log)) {
			TBC_PROCESS_UNLOCK_CHILD ();

			vortex_connection_shutdown (conn);
			turbulence_child_unref (child);
			return;
		} /* end if */

		/* socket that is know handled by the child process */
		client_socket = vortex_connection_get_socket (conn);

		/* send the socket descriptor to the child to avoid
		   holding a bucket in the parent */
		if (! turbulence_process_send_socket (client_socket, child, "s", 1)) {
			error ("Unable to send socket associated to the connection that originated the child process");
			TBC_PROCESS_UNLOCK_CHILD ();

			vortex_connection_shutdown (conn);
			turbulence_child_unref (child);
			return;
		}

		/* terminate the connection */
		vortex_connection_set_close_socket (conn, axl_false);
		vortex_connection_shutdown (conn);

		/* register pipes to receive child logs */
		__turbulence_process_prepare_logging (ctx, axl_true, general_log, error_log, access_log, vortex_log);

		/* register the child process identifier */
		axl_hash_insert_full (ctx->child_process,
				      /* store child pid */
				      INT_TO_PTR (pid), NULL,
				      /* data and destroy func */
				      child, (axlDestroyFunc) turbulence_child_unref);

		/* update number of childs running this profile path */
		def->childs_running++;

		TBC_PROCESS_UNLOCK_CHILD ();

		/* record child */
		msg ("PARENT=%d: Created child process pid=%d (childs: %d)", getpid (), pid, turbulence_process_child_count (ctx));
		return;
	} /* end if */

	/* reconfigure pids */
	ctx->pid = getpid ();

	/* release connections received from parent (including
	   sockets) */
	msg ("CHILD: calling to release all (parent) connections but conn-id=%d", 
	     vortex_connection_get_id (conn));
	__turbulence_process_release_parent_connections (ctx, conn, child);   

	/* call to start turbulence process */
	msg ("CHILD: Starting child process with: %s %s --child %s --config %s", 
	     turbulence_child_cmd_prefix ? turbulence_child_cmd_prefix : "", 
	     turbulence_bin_path ? turbulence_bin_path : "", child->socket_control_path, ctx->config_path);

	/* prepare child cmd prefix if defined */
	if (turbulence_child_cmd_prefix) {
		msg ("CHILD: found child cmd prefix: '%s', processing..", turbulence_child_cmd_prefix);
		cmds = axl_split (turbulence_child_cmd_prefix, 1, " ");
		if (cmds == NULL) {
			error ("CHILD: failed to allocate memory for commands");
			exit (-1);
		} /* end if */

		/* count positions */
		while (cmds[iterator] != 0)
			iterator++;

		/* expand to include additional commands */
		cmds = axl_realloc (cmds, sizeof (char*) * (iterator + 10 + 1));

		cmds[iterator] = (char *) turbulence_bin_path;
		iterator++;
		cmds[iterator] = "--child";
		iterator++;
		cmds[iterator] = child->socket_control_path;
		iterator++;
		cmds[iterator] = "--config";
		iterator++;
		cmds[iterator] = ctx->config_path;
		iterator++;
		if (turbulence_log_enabled (ctx)) {
			cmds[iterator] = "--debug";
			iterator++;
		}
		if (turbulence_log2_enabled (ctx)) {
			cmds[iterator] = "--debug2";
			iterator++;
		} 
		if (turbulence_log3_enabled (ctx)) {
			cmds[iterator] = "--debug3";
			iterator++;
		}
		if (ctx->console_color_debug) {
			cmds[iterator] = "--color-debug";
			iterator++;
		}
		if (__turbulence_module_no_unmap) {
			cmds[iterator] = "--no-unmap-modules";
			iterator++;
		}
		cmds[iterator] = NULL;

		/* run command with prefix */
		error_code = execvp (cmds[0], cmds);
	} else {

		/* run command without prefixes */
		error_code = execlp (
			/* configure command to run turbulence */
			turbulence_bin_path, "turbulence", "--child", child->socket_control_path,
			/* pass configuration file used */
			"--config", ctx->config_path,
			/* pass debug options to child */
			turbulence_log_enabled (ctx) ? "--debug" : "", 
			turbulence_log2_enabled (ctx) ? "--debug2" : "", 
			turbulence_log3_enabled (ctx) ? "--debug3" : "", 
			ctx->console_color_debug ? "--color-debug" : "",
			/* unmap modules support */
			__turbulence_module_no_unmap ? "--no-unmap-modules" : "",
			/* always last parameter */
			NULL);
	}
	
	error ("CHILD: unable to create child process, error found was: %d: errno: %s", error_code, vortex_errno_get_last_error ());
	exit (-1);

	/**** CHILD PROCESS CREATION FINISHED ****/
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
		TBC_PROCESS_LOCK_CHILD ();

		/* reacquire current childs */
		childs = axl_hash_items (ctx->child_process);

		/* notify all childs they will be closed */
		axl_hash_foreach (ctx->child_process, __terminate_child, ctx);


		/* unlock the list during the kill and wait */
		TBC_PROCESS_UNLOCK_CHILD ();

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

	TBC_PROCESS_LOCK_CHILD ();
	count = axl_hash_items (ctx->child_process);
	TBC_PROCESS_UNLOCK_CHILD ();

	msg ("child process count: %d..", count);
	return count;
}

axl_bool turbulence_process_child_list_build (axlPointer _pid, axlPointer _child, axlPointer _result)
{
	TurbulenceChild * child = _child;

	/* acquire a reference */
	if (turbulence_child_ref (child)) {
		/* add the child */
		axl_list_append (_result, child);
	} /* end if */
	
	return axl_false; /* do not stop foreach process */
}

/** 
 * @brief Allows to get the list of childs created at the time the
 * call happend.
 *
 * @param ctx The context where the childs were created. The context
 * must represents a master process (turbulence_ctx_is_child (ctx) ==
 * axl_false).
 *
 * @return A list (axlList) where each position contains a \ref
 * TurbulenceChild reference. Use axl_list_free to finish the list
 * returned. The function returns NULL if wrong reference received (it
 * is null or it does not represent a master process).
 */
axlList         * turbulence_process_child_list (TurbulenceCtx * ctx)
{
	axlList * result;

	if (ctx == NULL)
		return NULL;
	if (turbulence_ctx_is_child (ctx))
		return NULL;

	/* build the list */
	result = axl_list_new (axl_list_always_return_1, (axlDestroyFunc) turbulence_child_unref);
	if (result == NULL)
		return NULL;

	/* now add childs */
	TBC_PROCESS_LOCK_CHILD ();
	axl_hash_foreach (ctx->child_process, turbulence_process_child_list_build, result);
	TBC_PROCESS_UNLOCK_CHILD ();

	return result;
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
axl_bool turbulence_process_child_exists  (TurbulenceCtx * ctx, int pid)
{
	/* check context received */
	if (ctx == NULL || pid < 0)
		return axl_false;

	return (turbulence_process_find_pid_from_ppath_id (ctx, pid) != -1);
}

/** 
 * @brief Allows to get a reference to the child object identified
 * with the provided pid.
 *
 * @param ctx The context where the child process will be searched.
 *
 * @param pid The child pid process searched.
 *
 * @return A reference to the turbulence child process or NULL if it
 * fails. The caller must release the reference when no longer
 * required calling to \ref turbulence_child_unref.
 */
TurbulenceChild * turbulence_process_child_by_id (TurbulenceCtx * ctx, int pid)
{
	TurbulenceChild * child;

	/* check ctx reference */
	if (ctx == NULL)
		return NULL;

	/* lock */
	TBC_PROCESS_LOCK_CHILD ();

	/* get child */
	child = axl_hash_get (ctx->child_process, INT_TO_PTR (pid));

	/* unlock */
	TBC_PROCESS_UNLOCK_CHILD ();

	return NULL; /* no child found */
}

/** 
 * @internal Function used by
 * turbulence_process_find_pid_from_ppath_id
 */
axl_bool __find_ppath_id (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2) 
{
	int             * pid        = (int *) user_data;
	int             * _ppath_id  = (int *) user_data2;
	TurbulenceChild * child      = data;
	
	if (child->pid == (*pid)) {
		/* child found, update pid to have ppath_id */
		(*_ppath_id) = turbulence_ppath_get_id (child->ppath);
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
	
	TBC_PROCESS_LOCK_CHILD ();
	axl_hash_foreach2 (ctx->child_process, __find_ppath_id, &pid, &ppath_id);
	TBC_PROCESS_UNLOCK_CHILD ();

	/* check that the pid was found */
	return ppath_id;
	
}

/** 
 * @internal Function used by
 * turbulence_process_get_child_from_ppath
 */
axl_bool __find_ppath (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2) 
{
	TurbulencePPathDef  * ppath      = user_data;
	TurbulenceChild     * child      = data;
	TurbulenceChild    ** result     = user_data2;
	
	if (turbulence_ppath_get_id (child->ppath) == turbulence_ppath_get_id (ppath)) {
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
	
	/* lock mutex if signaled */
	if (acquire_mutex) {
		TBC_PROCESS_LOCK_CHILD ();
	}

	axl_hash_foreach2 (ctx->child_process, __find_ppath, def, &result);

	/* unlock mutex if signaled */
	if (acquire_mutex) {
		TBC_PROCESS_UNLOCK_CHILD ();
	}

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

/** 
 * @internal Set current turbulence binary path that was used to load
 * current turbulence process. You must pass an static value.
 */
void              turbulence_process_set_file_path (const char * path)
{
	/* set binary path */
	turbulence_bin_path = path;
	return;
}

/** 
 * @internal Set current turbulence child cmd prefix to be added to
 * child startup command. This may be used to add debugger
 * commands. You must pass an static value.
 */
void              turbulence_process_set_child_cmd_prefix (const char * cmd_prefix)
{
	/* set child cmd prefix */
	turbulence_child_cmd_prefix = cmd_prefix;
	return;
}

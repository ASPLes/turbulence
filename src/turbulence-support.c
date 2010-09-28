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
#include <turbulence-ctx-private.h>

#if defined(DEFINE_MKSTEMP_PROTO)
int mkstemp(char *template);
#endif

/** 
 * \defgroup turbulence_support Turbulence Support : support functions and useful APIs
 */

/** 
 * \addtogroup turbulence_support
 * @{
 */


#define write_and_check(str, len) do {					\
	if (write (temp_file, str, len) != len) {			\
		error ("Unable to write expected string: %s", str);     \
		close (temp_file);					\
		axl_free (temp_name);					\
		axl_free (str_pid);					\
		return NULL;						\
	}								\
} while (0)

/** 
 * @brief Allows to get process backtrace (including all threads) of
 * the given process id.
 *
 * @param ctx The context where the operation is implemented.
 *
 * @param pid The process id for which the backtrace is requested. Use
 * getpid () to get current process id.
 *
 * @return A newly allocated string containing the path to the file
 * where the backtrace was generated or NULL if it fails.
 */
char          * turbulence_support_get_backtrace (TurbulenceCtx * ctx, int pid)
{
#if defined(AXL_OS_UNIX)
	int                  temp_file;
	char               * temp_name;
	char               * str_pid;
	char               * command;
	int                  status;
	char               * backtrace_file;

	temp_name = axl_strdup ("/tmp/turbulence-backtrace.XXXXXX");
	temp_file = mkstemp (temp_name);
	if (temp_file == -1) {
		error ("Bad signal found but unable to create gdb commands file to feed gdb");
		return NULL;
	} /* end if */

	str_pid = axl_strdup_printf ("%d", getpid ());
	if (str_pid == NULL) {
		error ("Bad signal found but unable to get str pid version, memory failure");
		close (temp_file);
		return NULL;
	}
	
	/* write personalized gdb commands */
	write_and_check ("attach ", 7);
	write_and_check (str_pid, strlen (str_pid));

	axl_free (str_pid);
	str_pid = NULL;

	write_and_check ("\n", 1);
	write_and_check ("set pagination 0\n", 17);
	write_and_check ("thread apply all bt\n", 20);
	write_and_check ("quit\n", 5);
	
	/* close temp file */
	close (temp_file);
	
	/* build the command to get gdb output */
	backtrace_file = axl_strdup_printf ("%s/turbulence-backtrace.%d.gdb", turbulence_runtime_datadir (ctx), time (NULL));

	/* place some system information */
	command  = axl_strdup_printf ("echo \"Turbulence backtrace at `hostname -f`, created at `date`\" > %s", backtrace_file);
	status   = system (command);
	axl_free (command);

	/* get profile path id */
	if (ctx->is_main_process) 
		command  = axl_strdup_printf ("echo \"Failure found at main process.\" >> %s", backtrace_file);
	else {
		/* get profile path associated to child process */
		command   = axl_strdup_printf ("echo \"Failure found at child process.\" >> %s", backtrace_file);
	}
	status   = system (command);
	axl_free (command);

	/* get place some pid information */
	command  = axl_strdup_printf ("echo -e 'Process that failed was %d. Here is the backtrace:\n--------------' >> %s", getpid (), backtrace_file);
	status   = system (command);
	axl_free (command);
	
	/* get backtrace */
	command  = axl_strdup_printf ("gdb -x %s >> %s", temp_name, backtrace_file);
	status   = system (command);
	
	/* remove gdb commands */
	unlink (temp_name);
	axl_free (temp_name);
	axl_free (command);

	/* return backtrace file created */
	return backtrace_file;

#elif defined(AXL_OS_WIN32)
	error ("Backtrace for Windows not implemented..");
	return NULL;
#endif			
}

axl_bool turbulence_support_smtp_send_receive_reply_and_check (TurbulenceCtx * ctx,
							       VORTEX_SOCKET   conn, 
							       char          * buffer, 
							       int             buffer_size, 
							       const char    * error_message)
{
	int read_bytes;

	/* clear buffer received */
	memset (buffer, 0, buffer_size);

	/* receive content */
	read_bytes = recv (conn, buffer, buffer_size, 0);
	if (read_bytes <= 0) {
		error ("Failed to receive reply SMTP content. %s", error_message);
		return axl_false;
	} /* end if */

	buffer[read_bytes - 1] = 0;

	if (! axl_memcmp (buffer, "250", 3) && ! axl_memcmp (buffer, "220", 3) && ! axl_memcmp (buffer, "354", 3)) {
		error ("Received negative SMTP reply: %s", buffer);
		vortex_close_socket (conn);
		return axl_false;
	}

	msg ("Received afirmative SMTP content: %s", buffer);
	return axl_true;
}

/** 
 * @brief Allows to send a mail message through the provided smtp
 * server and port, with the provided content.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param mail_from The mail from value to configure on this mail
 * message. If NULL is provided, "turbulence@localdomain.local" will
 * be used.
 *
 * @param mail_to The destination address value to configure on this
 * mail message. This value is not optional and must be defined and
 * pointing to a right account.
 *
 * @param subject The message subject to be configured. This value is
 * optional, if not configured no subject will be placed.
 *
 * @param body The message body to be configured. This value is
 * optional, if not configured no subject will be placed.
 *
 * @param body_file Optional reference to a file that contains the
 * body of the message.
 *
 * @param smtp_server The location of the smtp server. If NULL is passed, localhost will be used.
 *
 * @param smtp_port The port location of the smtp server. If NULL is passed, 25 will be used.
 *
 * @return axl_true in the case the mail message was successfully sent
 * otherwise axl_false is returned.
 */
axl_bool        turbulence_support_smtp_send (TurbulenceCtx * ctx, 
					      const char    * mail_from,
					      const char    * mail_to,
					      const char    * subject,
					      const char    * body,
					      const char    * body_file,
					      const char    * smtp_server,
					      const char    * smtp_port)
{
	VORTEX_SOCKET   conn;
	axlError      * err = NULL;
	char            buffer[1024];
	FILE          * body_ffile;
	int             bytes_read, bytes_sent;

	if (ctx == NULL || mail_to == NULL)
		return axl_false;

	/* set a default mail_from */
	if (mail_from == NULL)
		mail_from = "turbulence@localdomain.local";

	/* create connection */
	conn = vortex_connection_sock_connect (TBC_VORTEX_CTX (ctx), 
					       smtp_server ? smtp_server : "localhost",
					       smtp_port ? smtp_port : "25",
					       NULL, &err);
	if (conn == -1) {
		error ("Unable to connect to smtp server %s:%s, error was: %s",
		       smtp_server ? smtp_server : "localhost",
		       smtp_port ? smtp_port : "25",
		       axl_error_get (err));
		axl_error_free (err);
	} /* end if */

	/* read greetings */
	if (! turbulence_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive initial SMTP greetings"))
		return axl_false;

	/* ok, now write content */
	if (send (conn, "mail from: ", 11, 0) != 11) {
		error ("Unable to send mail message, failed to send mail from content..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_from, strlen (mail_from), 0) != strlen (mail_from)) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! turbulence_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive mail from confirmation"))
		return axl_false;

	if (send (conn, "rcpt to: ", 9, 0) != 9) {
		error ("Unable to send mail message, failed to send rcpt to content..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_to, strlen (mail_to), 0) != strlen (mail_to)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! turbulence_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive rcpt to confirmation"))
		return axl_false;

	/* init data section */
	if (send (conn, "data\n", 5, 0) != 5) {
		error ("Unable to send mail message, failed to send data section..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! turbulence_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive data start section confirmation"))
		return axl_false;

	/* send To: content */
	if (send (conn, "To: ", 4, 0) != 4) {
		error ("Unable to send mail message, failed to send To: content..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_to, strlen (mail_to), 0) != strlen (mail_to)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send From: content */
	if (send (conn, "From: ", 6, 0) != 6) {
		error ("Unable to send mail message, failed to send From: content..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_from, strlen (mail_from), 0) != strlen (mail_from)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */
	
	/* check subject */
	if (subject) {
		/* send subject content */
		if (send (conn, "Subject: ", 9, 0) != 9) {
			error ("Unable to send mail message, failed to send subject section..");
			vortex_close_socket (conn);
			return axl_false;
		} /* end if */

		/* send the subject it self */
		if (send (conn, subject, strlen (subject), 0) != strlen (subject)) {
			error ("Unable to send mail message, failed to send subject content..");
			vortex_close_socket (conn);
			return axl_false;
		} /* end if */


		/* send termination */
		if (send (conn, "\r\n", 2, 0) != 2) {
			error ("Unable to send mail message, failed to send mail from content (address)..");
			vortex_close_socket (conn);
			return axl_false;
		} /* end if */
	}

	/* now send body */
	if (body) {
		if (send (conn, body, strlen (body), 0) != strlen (body)) {
			error ("Unable to send mail message, failed to send body content..");
			vortex_close_socket (conn);
			return axl_false;
		} /* end if */

		/* send termination */
		if (send (conn, "\r\n", 2, 0) != 2) {
			error ("Unable to send mail message, failed to send mail from content (address)..");
			vortex_close_socket (conn);
			return axl_false;
		} /* end if */
	}

	/* check for body from file */
	if (body_file) {
		body_ffile = fopen (body_file, "r");
		if (body_ffile) {
			/* read and send content */
			bytes_read = fread (buffer, 1, 1024, body_ffile);
			while (bytes_read > 0) {

				/* send content */
				bytes_sent = send (conn, buffer, bytes_read, 0);
				if (bytes_sent != bytes_read) {
					error ("Unable to send mail message, found a failure while sending content from file (sent %d != requested %d)..",
					       bytes_sent, bytes_read);
					fclose (body_ffile);
					vortex_close_socket (conn);
					return axl_false;
				} /* end if */

				/* read next content */
				bytes_read = fread (buffer, 1, 1024, body_ffile);
			} /* end while */
			fclose (body_ffile);
		} /* end if */
	} /* end if */

	/* send termination */
	if (send (conn, ".\r\nquit\r\n", 8, 0) != 8) {
		error ("Unable to send mail message, failed to send termination message..");
		vortex_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! turbulence_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive end message confirmation"))
		return axl_false;

	return axl_true;
}

/** 
 * @brief Allows to send a SMTP message using the configuration found
 * on the provided smtp_conf declaration. This smtp_conf declaration
 * is found at the turbulence configuration file. See \ref
 * turbulence_smtp_notifications
 *
 * @param ctx The turbulence context where the operation will be
 * implemented.
 *
 * @param smtp_conf_id The string identifying the smtp configuration (id
 * declaration inside <smtp-server> node) or NULL. If NULL is used,
 * then the first smtp server with is-default=yes declared is used.
 *
 * @param subject Optional subject to be configured on mail body
 * message.
 * 
 * @param body The message body to be configured. If NULL is provided
 * no body will be sent.
 *
 * @param body_file Optional reference to a file that contains the
 * body of the message.
 *
 * @return axl_true if the mail message was submited or axl_false if
 * something failed.
 */
axl_bool        turbulence_support_simple_smtp_send (TurbulenceCtx * ctx,
						     const char    * smtp_conf_id,
						     const char    * subject,
						     const char    * body,
						     const char    * body_file)
{
	axlNode * node;
	axlNode * default_node;

	if (ctx == NULL)
		return axl_false;

	/* find smtp mail notification conf */
	node         = axl_doc_get (turbulence_config_get (ctx), "/turbulence/global-settings/notify-failures/smtp-server");
	default_node = NULL;
	while (node) {
		/* check for declaration with the smtp conf requested */
		if (HAS_ATTR_VALUE (node, "id", smtp_conf_id))
			break;

		/* check for default node declaration */
		if (HAS_ATTR_VALUE (node, "is-default", "yes") && default_node == NULL)
			default_node = node;

		/* node not found, go next */
		node = axl_node_get_next_called (node, "smtp-server");
	} /* end while */

	/* set to default node found (if any) when node is null */
	if (node == NULL)
		node = default_node;

	/* check if the smtp configuration was found */
	if (node == NULL) {
		error ("Failed to send mail notification, unable to find default smtp configuration or smtp configuration associated to id '%s'",
		       smtp_conf_id ? smtp_conf_id : "NULL");
		return axl_false;
	}
	
	/* now use SMTP send values */
	return turbulence_support_smtp_send (ctx, ATTR_VALUE (node, "mail-from"), 
					     ATTR_VALUE (node, "mail-to"), subject, body, body_file, 
					     ATTR_VALUE (node, "server"), ATTR_VALUE (node, "port"));
}


/** 
 * @}
 */

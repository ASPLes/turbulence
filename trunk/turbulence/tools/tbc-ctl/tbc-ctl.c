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

/* include axl support */
#include <axl.h>

/* include exarg support */
#include <exarg.h>

/* include turbulence */
#include <turbulence.h>

/* include sasl support */
#include <vortex_sasl.h>

/* include read line support */
#include <readline/readline.h>
#include <readline/history.h>

#define HELP_HEADER "tbc-ctl: a CLI tool to manage turbulence remote management interface\n\
Copyright (C) 2009  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER ""

/* " */

/** 
 * @brief Default turbulence and vortex context.
 */
TurbulenceCtx    * ctx;
VortexCtx        * vortex_ctx;
VortexConnection * conn;

axlList          * commands = NULL;

axl_bool           debug_was_enabled = axl_false;

typedef struct _TbcCtlCommand {
	char * command;
	char * description;
} TbcCtlCommand;

void tbc_ctl_command_free (axlPointer _data)
{
	TbcCtlCommand * command = (TbcCtlCommand *) _data;
	axl_free (command->command);
	axl_free (command->description);
	axl_free (command);
	return;
}

/** 
 * @brief Profile that defines radmin channels
 */
#define RADMIN_URI "urn:aspl.es:beep:profiles:radmin-ctl"

#if !defined(getpass)
/* add a prototype to avoid a warning */
char *getpass( const char * prompt );
#endif 

#ifdef AXL_OS_WIN32
char     tbc_ctl_getch (void )
{

	HANDLE hInput;
	char   ch;
	int   n;

	/* get the STDIN handle. */
	hInput = GetStdHandle(STD_INPUT_HANDLE);
	
	/* turn off echoing */
        SetConsoleMode(hInput, ENABLE_PROCESSED_INPUT);
	
	/* reac one char from the console */
	ReadConsole(hInput, &ch, 1, (DWORD *) &n, NULL);

	return ch;
}
#endif

/** 
 * @brief Allows to get a password from the user writting to stdout hash simbols.
 * 
 * @param prompt The prompt to be showed to the user.
 * 
 * @return The password get. Do not free returned value. If you need a
 * newly allocated value use \ref afdal_getpass_allocated.
 */
char  * tbc_ctl_getpass (char  * prompt) 
{
#ifdef AXL_OS_UNIX
	char  * s;
#endif
#ifdef AXL_OS_WIN32	
#define MAX_LEN 30
	int character, length, maxlen = MAX_LEN;
	static char s[MAX_LEN], *position;
#endif

	/* get the password unix way */
#ifdef AXL_OS_UNIX
	s =  getpass (prompt);
#endif

	/* get the password windows way */
#ifdef AXL_OS_WIN32	
	length   = 0;
	position = s;
	fprintf (stdout, prompt);

	while ((character = tbc_ctl_getch ()) != '\r' && 
	       (character != '\n') && (character != 27) && (length <= maxlen)) { 
		if (character == '\b')	{
			if ( length > 0 ) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				--length;
				--position;
			}
		}
		else if (character < 32 || character > 127);
		else {   
			putchar('#');
			*position++ = (char) character;
			++length;
		}
	}

	*position = '\0';
	putchar ('\n');

#endif
	/* function end */
	return s;
}

void tbc_ctl_connection_closed (VortexConnection * conn)
{
	
	printf ("\n**\n** Connection=%d to server closed\n**\n", vortex_connection_get_id (conn));

	return;
}


axl_bool tbc_ctl_do_connection (void) {

	const char * host = "localhost";
	const char * port = "602";

	/* try first to create a connection using default values or
	   user provided values */
	if (exarg_is_defined ("host"))
		host = exarg_get_string ("host");
	if (exarg_is_defined ("port"))
		port = exarg_get_string ("port");
	
	msg ("connecting turbulence at %s:%s..", host, port);
	conn = vortex_connection_new (vortex_ctx, 
				      host, 
				      port,
				      NULL, NULL);
	/* check connection returned */
	if (! vortex_connection_is_ok (conn, axl_false))
		return axl_false;

	/* install on close handling */
	vortex_connection_set_on_close (conn, tbc_ctl_connection_closed);

	return axl_true;
}

/** 
 * @internal Common parsing handling to check errors returned by
 * remote turbulence process.
 */
axlDoc * tbc_ctl_parse_content_and_check_errors (VortexFrame * frame)
{
	axlDoc        * doc;
	axlError      * err = NULL;
	axlNode       * node;
	const char    * content;
	

	/* parse content returned */
	doc = axl_doc_parse ((const char *) vortex_frame_get_payload (frame), vortex_frame_get_payload_size (frame), &err);
	
	if (doc == NULL) {
		printf (" ERROR code:    6\n");
		printf (" ERROR msg:     Unable to parse reply received from turbulence server.\n");
		printf (" ERROR error:   %d:%s\n", axl_error_get_code (err), axl_error_get (err));
		axl_error_free (err);
		return NULL;
	} /* end if */

	/* document parsed ok, check status */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ERR) {
		/* get root node */
		node = axl_doc_get_root (doc);

		/* found error */
		printf (" ERROR code:     %s\n", ATTR_VALUE (node, "code"));
		printf (" ERROR msg:      %s\n", axl_node_get_content_trans (node, NULL));

		/* get error content */
		node = axl_node_get_next_called  (node, "content");
		content = axl_node_get_content (node, NULL);
		if (content != NULL && strlen (content) > 0)
			printf (" ERROR content:  %s\n", content);

		/* free document */
		axl_doc_free (doc);
		return NULL;
	}

	/* document seems ok, return it */
	return doc;
}

void tbc_ctl_frame_received (VortexChannel    * channel, 
			     VortexConnection * connection,
			     VortexFrame      * frame,
			     axlPointer         user_data)
{
	axlDoc     * doc;

	/* parse content received and check errors */
	doc = tbc_ctl_parse_content_and_check_errors (frame);
	if (doc == NULL)
		return;

	/* free document received */
	axl_doc_free (doc);
	return;
}

axl_bool tbc_ctl_enable_sasl (void)
{
	char         * mech = axl_strdup (VORTEX_SASL_PLAIN);
	VortexStatus   status;
	char         * message = NULL;
	char         * string;

	/* init sasl */
	if (! vortex_sasl_init (vortex_ctx)) {
		printf ("ERROR: failed to initialize SASL module..unable to authenticate\n");
		return axl_false;
	}

	/* get plan mechamis */
	if (exarg_is_defined ("sasl-method"))
		mech = axl_strdup_printf ("%s/%s", VORTEX_SASL_FAMILY, axl_stream_to_upper (exarg_get_string ("sasl-method")));
	msg ("using SASL auth %s", mech);
	
	/* get auth properies */
	if (axl_cmp (mech, VORTEX_SASL_PLAIN)) {
		if (exarg_is_defined ("user"))
			vortex_sasl_set_propertie (conn, VORTEX_SASL_AUTH_ID, exarg_get_string ("user"), NULL);
		else {
			string = readline ("User: ");
			vortex_sasl_set_propertie (conn, VORTEX_SASL_AUTH_ID, string, axl_free);
		} /* end if */

		/* now password */
		if (exarg_is_defined ("password"))
			vortex_sasl_set_propertie (conn, VORTEX_SASL_PASSWORD, exarg_get_string ("password"), NULL);
		else {
			/* close stdout */
			string = tbc_ctl_getpass ("Password: ");
			string = axl_strdup (string);

			/* end if */
			vortex_sasl_set_propertie (conn, VORTEX_SASL_PASSWORD, string, axl_free);
		} /* end if */
	} /* end if */

	/* enable auth operation */
	vortex_sasl_start_auth_sync (conn, mech, &status, &message);

	if (status  == VortexOk) 
		msg ("SASL status: %s", message);
	else
		error ("SASL status: %s", message);

	/* free status message nad mech */
	axl_free (mech);

	/* return SASL auth status */
	return status == VortexOk;
}

/** 
 * @internal Function used to create management interface.
 */
axl_bool  tbc_ctl_create_management_channel (void) {
	VortexChannel * channel;
	char          * msg;
	int             code;

 create_channel:
	/* try first to create the channel without using SASL first */
	channel = vortex_channel_new (conn,
				      0, 
				      RADMIN_URI,
				      /* no close handler */
				      NULL, NULL, 
				      /* frame received */
				      tbc_ctl_frame_received, NULL,
				      NULL, NULL);
	if (channel == NULL) {
		error ("Failed to create channel, error was: ");
		while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
			error ("  %d: %s", code, msg);
			axl_free (msg);
		} /* end while */

		/* check if the channel is already authenticated */
		if (vortex_sasl_is_authenticated (conn))
			return axl_false;

	} else {
		/* channel created */
		return axl_true;
	} /* end if */

	/* ok, now try enable SASL auth */
	if (! tbc_ctl_enable_sasl ()) 
		return axl_false;
	
	/* try again to create the channel */
	goto create_channel;
	return axl_false;
}

void tbc_refresh_available_commands (axlDoc * doc)
{
	axlNode       * node;
	axlNode       * command;
	TbcCtlCommand * cmd;

	/* create the list of commands */
	if (commands != NULL)
		axl_list_free (commands);
	commands = axl_list_new (axl_list_always_return_1, tbc_ctl_command_free);

	/* add virtual command */
	cmd = axl_new (TbcCtlCommand, 1);
	cmd->command = axl_strdup ("debug");
	cmd->description = axl_strdup ("Enable debug during this session");

	/* insert into the list */
	axl_list_append (commands, cmd);	
	
	/* for each command registered */
	node = axl_doc_get (doc, "/table/content/row");
	while (node != NULL) {
		/* get command node (first column) */
		command = axl_node_get_child_called (node, "d");
		
		/* create a command node */
		cmd              = axl_new (TbcCtlCommand, 1);
		cmd->command     = axl_strdup (axl_node_get_content (command, NULL));
		
		/* get description */
		command = axl_node_get_next_called (command, "d");
		cmd->description = axl_strdup (axl_node_get_content (command, NULL));

		/* insert into the list */
		axl_list_append (commands, cmd);

		/* call to get next node */
		node = axl_node_get_next_called (node, "row");
	} /* end while */

	return;
}

void tbc_ctl_update_commands_available (void)
{
	VortexChannel * channel;
	WaitReplyData * wait_reply;
	int             msg_no;
	axlDoc        * doc;
	VortexFrame   * frame;

	/* ask turbulence server to send back a list of commands that
	   can be used */
	channel = vortex_connection_get_channel_by_uri (conn, RADMIN_URI);
	if (channel == NULL) {
		error ("Unable to find %s channel, failed to get available commands..",
		       RADMIN_URI);
		return;
	} /* end if */

	/* create wait reply to handle this particular reply out of
	   the usual context */
	wait_reply = vortex_channel_create_wait_reply ();
	
	/* now send command */
	if (! vortex_channel_send_msg_and_wait (channel, "<request operation='commands available' />", 44, &msg_no, wait_reply)) {
		error ("Unable to send commands available request..");
		return;
	}
	frame = vortex_channel_wait_reply (channel, msg_no, wait_reply);
	
	/* parse content received and check errors */
	doc = tbc_ctl_parse_content_and_check_errors (frame);
	vortex_frame_unref (frame);
	if (doc == NULL) 
		return;

	/* install commands available */
	tbc_refresh_available_commands (doc);

	/* free document */
	axl_doc_free (doc);

	return;
}

void tbc_ctl_pritn_content_received_table (axlDoc * doc)
{
	axlNode    * node;
	axlNode    * node2;
	int          num_columns = 0;
	int          size;
	int          length;
	int          col_lengths[20];
	int          iterator;
	const char * content;   

	/* print title */
	node = axl_doc_get (doc, "/table/title");
	if (node) 
		printf (">>> %s <<<\n", axl_node_get_content (node, &size));

	/* get initial column header sizes */
	node     = axl_doc_get (doc, "/table/column-description/column");
	iterator = 0;
	while (node) {
		/* get initial column length */
		col_lengths[iterator] = strlen (ATTR_VALUE (node, "name"));

		/* get next column */
		node = axl_node_get_next_called (node, "column");

		/* increase number of columns found */
		num_columns++;

		/* next iterator */
		iterator++;

	} /* end while */

	if (num_columns > 0)
		printf ("\n");

	/* now ensure we have maximum lengths */
	node = axl_doc_get (doc, "/table/content/row");
	while (node) {

		/* now print each column */
		iterator = 0;
		node2    = axl_node_get_child_called (node, "d");
		while (node2) {
			content = axl_node_get_content (node2, &size);
			if (size > col_lengths[iterator]) {
				col_lengths[iterator] = size;
			}

			/* get next node called */
			node2 = axl_node_get_next_called (node2, "d");

			/* next iterator */
			iterator++;
		} /* end while */

		/* get next node */
		node = axl_node_get_next_called (node, "row");
	} /* end if */

	/* now print column headers */
	node     = axl_doc_get (doc, "/table/column-description/column");
	iterator = 0;
	while (node) {
		/* print column name */
		printf ("%s", ATTR_VALUE (node, "name"));

		length = strlen (ATTR_VALUE (node, "name"));
		while (length < col_lengths[iterator]) {
			printf (" ");
			length++;
		}
		printf ("   ");

		/* get next column */
		node = axl_node_get_next_called (node, "column");

		/* next iterator */
		iterator++;

	} /* end while */

	if (num_columns > 0)
		printf ("\n");

	/* now print columns separators */
	iterator = 0;
	node     = axl_doc_get (doc, "/table/column-description/column");
	while (node) {
		/* print column name */
		length = col_lengths[iterator];
		while (length > 0) {
			printf ("-");
			length--;
		}
		printf ("   ");

		/* get next column */
		node = axl_node_get_next_called (node, "column");
		iterator++;

	} /* end while */
	
	if (num_columns > 0)
		printf ("\n");

	/* now print the content */
	node = axl_doc_get (doc, "/table/content/row");
	while (node) {

		/* now print each column */
		node2    = axl_node_get_child_called (node, "d");
		iterator = 0;
		while (node2) {
			/* print content */
			printf ("%s", axl_node_get_content (node2, &size));

			while (size < col_lengths[iterator]) {
				printf (" ");
				size++;
			}
			printf ("   ");

			/* get next node called */
			node2 = axl_node_get_next_called (node2, "d");

			/* next iterator */
			iterator++;
		} /* end while */

		printf ("\n");

		/* get next node */
		node = axl_node_get_next_called (node, "row");
	} /* end if */

	return;
}

void tbc_ctl_print_content_received (axlDoc * doc)
{
	axlNode * node = axl_doc_get_root (doc);

	/* check result type */
	if (NODE_CMP_NAME (node, "table")) {
		tbc_ctl_pritn_content_received_table (doc);
	}
	
	return;
}

void tbc_ctl_command_send (const char * command)
{
	VortexChannel * channel;
	WaitReplyData * wait_reply;
	int             msg_no;
	VortexFrame   * frame;
	axlDoc        * doc;

	if (command == NULL || strlen (command) == 0)
		return;

	/* ask turbulence server to send back a list of commands that
	   can be used */
	channel = vortex_connection_get_channel_by_uri (conn, RADMIN_URI);
	if (channel == NULL) {
		error ("Unable to find %s channel, failed to get available commands..",
		       RADMIN_URI);
		return;
	} /* end if */

	/* create wait reply to handle this particular reply out of
	   the usual context */
	wait_reply = vortex_channel_create_wait_reply ();

	/* send command */
	if (! vortex_channel_send_msg_and_waitv (channel, &msg_no, wait_reply, "<request operation='%s' />", command)) {
		error ("Unable to send commands available request..");
		return;
	}

	/* wait for reply */
	frame = vortex_channel_wait_reply (channel, msg_no, wait_reply);

	if (debug_was_enabled)  
		printf ("DEBUG: content received: %s\n", frame ? (char *) vortex_frame_get_payload (frame) : "null frame received");
	
	/* parse content received and check errors */
	doc = tbc_ctl_parse_content_and_check_errors (frame);
	if (doc == NULL) 
		return;

	/* content received */
	tbc_ctl_print_content_received (doc);

	/* free document received */
	axl_doc_free (doc);

	return;
}

void tbc_ctl_print_commands (void)
{ 
	int             iterator = 0;
	TbcCtlCommand * cmd;


	printf ("List of commands available:\n\n");
	printf ("help -- Get this help\n");
	printf ("debug -- Enable debug during this session\n");

	while (iterator < axl_list_length (commands)) {
		/* get command */
		cmd = axl_list_get_nth (commands, iterator);
		
		printf ("%s -- %s\n", cmd->command, cmd->description);

		/* next command */
		iterator++;
	} /* end while */

	printf ("\n");

	return;
}


void tbc_ctl_command_loop (void)
{
	char * command;
	char * prompt = NULL;

	/* get commands available */
	tbc_ctl_update_commands_available ();

	while (axl_true) {
		/* build connection prompt */
		axl_free (prompt);
		if (vortex_connection_is_ok (conn, axl_false)) {
			prompt = axl_strdup_printf ("tbc-ctl:%s:%s> ", 
						    vortex_connection_get_host (conn), vortex_connection_get_port (conn));
		} else {
			prompt = axl_strdup ("tbc-ctl (unconnected)> ");
		} /* end if */

		/* read command */
		command = readline (prompt);

		/* process command */
		if (command == NULL || axl_cmp (command, "quit") || axl_cmp (command, "exit")) {
			axl_free (command);
			msg ("Exiting..");
			break;
		}

		/* check for empty commands */
		axl_stream_trim (command);
		if (strlen (command) == 0) {
			axl_free (command);
			continue;
		}

		/* save the command */
		add_history (command);

		/* detect help command */
		if (axl_cmp (command, "help")) {
			/* print available commands */
			tbc_ctl_print_commands ();
		} else if (axl_cmp (command, "debug")) {
			/* invert current selection */
			debug_was_enabled = ! debug_was_enabled;

			/* enable debug */
			turbulence_log_enable   (ctx, debug_was_enabled);
			turbulence_log2_enable  (ctx, debug_was_enabled);
			turbulence_color_log_enable (ctx, debug_was_enabled);

			printf ("Debug was: %s\n", debug_was_enabled ? "ENABLED" : "disabled");

		} else {
			/* send command read */
			tbc_ctl_command_send (command);
		}
		
		/* free command */
		axl_free (command);

		/* FIXME: at this point check connection status */
		

	} /* end if */

	/* terminate prompt */
	axl_free (prompt);

	return;
}

int tbc_auto_completion_check (TbcCtlCommand ** last_matched, axl_bool show_command)
{
	int             matches = 0;
	int             iterator = 0;
	TbcCtlCommand * command;
	
	while (iterator < axl_list_length (commands)) {
		/* get next command */
		command = axl_list_get_nth (commands, iterator);
		
		/* check commands to be skipped */
		if (axl_cmp (command->command, "commands available")) {
			iterator++;
			continue;
		}
		
		/* check if the command maches */
		if (! axl_stream_casecmp (rl_line_buffer, command->command, rl_end)) {
			iterator++;
			continue;
		}

		/* check to show command */
		if (show_command)
			printf ("  %s : %s\n", command->command, command->description);
		
		/* check matches */
		matches++;

		/* update last matched */
		(*last_matched) = command;
		
		/* next iterator */
		iterator++;
	} /* end while */

	/* check to show command */
	if (show_command)
		printf ("%s%s", rl_prompt, rl_line_buffer);
	
	return matches;
}

int tbc_auto_completion (int count, int key)
{
	TbcCtlCommand * command = NULL;
	int             matches = 0;

	/* count matches */
	matches = tbc_auto_completion_check (&command, axl_false);
	if (matches == 1) {
		/* place exact command */
		rl_insert_text (&command->command[rl_point]);
		return 0;
	}
	
	/* show as much as commands matched */
	printf ("\n");
	printf ("  help [command]\n");
	tbc_auto_completion_check (&command, axl_true);
	return 0;
}

int main (int argc, char ** argv)
{


	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Provides tool version");

	exarg_install_arg ("host", "h", EXARG_STRING, 
			   "Configures turbulence host location (default localhost)");

	exarg_install_arg ("port", "p", EXARG_STRING, 
			   "Configures turbulence port location (default 602)");

	exarg_install_arg ("sasl-method", "s", EXARG_STRING, 
			   "Optionally configures default SASL authentication method to be used (default PLAIN if --user is used)");

	exarg_install_arg ("user", "u", EXARG_STRING, 
			   "Optionally configures SASL authentication user");

	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Allows to configure to enable log");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Allows to configure to enable second level log");

	exarg_install_arg ("color-debug", NULL, EXARG_NONE,
			   "Allows to configure color debug");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* init turbulence context */
	ctx        = turbulence_ctx_new ();
	vortex_ctx = vortex_ctx_new ();

	/* init vortex */
	if (! vortex_init_ctx (vortex_ctx))  {
		error ("Unable to initialize vortex context.."); 
		return -1;
	}
	
	/* bind vortex ctx */
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);

	/* configure context debug according to values received */
	turbulence_log_enable  (ctx, true);
	turbulence_log2_enable (ctx, exarg_is_defined ("debug2"));
	turbulence_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* check console color debug */
	turbulence_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* check version argument */
	if (exarg_is_defined ("version")) {
		/* free turbulence context */
		turbulence_ctx_free (ctx);

		msg ("%s version: %s", argv[0], VERSION);
		return 0;
	}

	/* configure read line */
	rl_bind_key ('\t', tbc_auto_completion);

	/* init application */
	if (! tbc_ctl_do_connection ()) {
		
		error ("Failed to perform connection application: %s..",
		       vortex_connection_get_message (conn));
		vortex_connection_close (conn);
		return -1;
	}
	msg ("connected OK!");

	/* create tbc-ctl management channel */
	if (! tbc_ctl_create_management_channel ()) {
		vortex_connection_close (conn);
		return -1;
	} /* end if */

	/* ok, we are now connected, loop reading user commands */
	tbc_ctl_command_loop ();

	/* close the connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* free commands */
	axl_list_free (commands);

	/* free context */
	turbulence_ctx_free (ctx);

	/* finish vortex */
	vortex_exit_ctx (vortex_ctx, axl_true);

	/* finish exarg */
	exarg_end ();

	return 0;
}

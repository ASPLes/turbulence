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
#include <mod_radmin.h>

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

axlDoc * mod_radmin_command_reload (const char * line, axlPointer user_data, axl_bool * status)
{
	axlDoc * doc;

	/* signal command returned proper status */
	(*status) = axl_true;

	/* call to reload */
	turbulence_reload_config (ctx, 0);

	/* return simple result */
	doc = axl_doc_parse ("<simple code='0' msg='reload ok' />", -1, NULL);

	return doc;
}

axlDoc * mod_ramdin_command_commands_available (const char * line, axlPointer user_data, axl_bool * status)
{
	int                    iterator   = 0;
	ModRadminCommandItem * item;
	axlDoc               * result;
	axlNode              * node;
	axlNode              * nodeAux;
	axlError             * error = NULL;

	/* signal command returned proper status */
	(*status) = axl_true;

	/* lock mutex */
	vortex_mutex_lock (&commands_mutex);

	/* result document */
	result = axl_doc_parse_strings (&error, 
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
		       axl_error_get_code (error), axl_error_get (error));
		axl_error_free (error);
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
 * @brief Function that handles an incoming request checking if there
 * is a command handler registered for the command operation received.
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
	char                 * str_result;
	int                    str_size;
	
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
					  "<error code='5'><msg>No command handler was found associated to %s</msg><content></content></error>",
					  command);
		return;
	} /* end if */

	/* ok, now call to handle command and return content */
	status = axl_false;
	result = cmd->handler (ATTR_VALUE (node, "arguments"), cmd->user_data, &status);

	if (! status) {
		vortex_channel_send_errv (channel,
					  vortex_frame_get_msgno (frame),
					  "<error code=\"4\"><msg>Command handler returned wrong status</msg><content></content></error>");
		/* free document returned by command handler */
		axl_doc_free (result);
		return;
	} 

	/* FIXME: implement here format conversion */

	/* build string representation */
	if (! axl_doc_dump (result, &str_result, &str_size)) {
		vortex_channel_send_errv (channel, 
					  vortex_frame_get_msgno (frame),
					  "<error code=\"8\"><msg>Command handler returned an xml document that failed to be dumped.</msg><content></content></error>");
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

/* mod_radmin init handler */
static int  mod_radmin_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

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

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_radmin",
	"Remote administration and status checking for Turbulence",
	mod_radmin_init,
	mod_radmin_close,
	mod_radmin_reconf,
	mod_radmin_unload
};

END_C_DECLS


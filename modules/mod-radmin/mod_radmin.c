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

typedef char * (*ModRadminCommandHandler) (const char * line, axlPointer user_data);

typedef struct _ModRadminCommandItem {
	char                     * command;
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

char * mod_radmin_command_show_status (const char * line, axlPointer user_data)
{
	
	return axl_strdup ("OK");
}

char * mod_radmin_command_reload (const char * line, axlPointer user_data)
{
	return axl_strdup ("reloading..");
}

/** 
 * @internal Function used to install commands into the list of
 * commands available.
 */
void mod_radmin_install_command (const char               * command, 
				 const char               * description,
				 ModRadminCommandHandler    handler)
{
	ModRadminCommandItem * cmd;

	/* lock mutex */
	vortex_mutex_lock (&commands_mutex);

	/* create command handler */
	cmd              = axl_new (ModRadminCommandItem, 1);
	cmd->command     = axl_strdup (command);
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
	
	mod_radmin_install_command ("show status", 
				    "Allows to show current global turbulence status",     
				    mod_radmin_command_show_status);
	mod_radmin_install_command ("reload", 
				    "Allows to instruct turbulence to reload its configuration", 
				    mod_radmin_command_reload);
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
	const char               * description = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);
	ModRadminCommandHandler    handler     = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);

	msg ("received request to install command");
	mod_radmin_install_command (command, description, handler);
	
	return;
}

void mod_radmin_frame_received (VortexChannel    * channel,
				VortexConnection * conn,
				VortexFrame      * frame,
				axlPointer         user_data)
{
	/* command received, handle request and reply */
	msg ("received command request: %s", vortex_frame_get_payload (frame));
	
	/* reply with content */
	vortex_channel_send_rpy (channel, "still not implemented..", 23, vortex_frame_get_msgno (frame));

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


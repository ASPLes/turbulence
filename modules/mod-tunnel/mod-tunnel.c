/*
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
 *         info@aspl.es - http://fact.aspl.es
 */
#include <mod-tunnel.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other task. The function must
 * return true to signal that the module was initialized
 * ok. Otherwise, false must be returned.
 */
static bool tunnel_init ()
{
	msg ("turbulence TUNNEL init");

	/* register the profile, using the basic handlers */
	vortex_profiles_register (
		/* profile uri */
		TUNNEL_PROFILE,
		/* tunnel starth handler, no basic start handler
		 * provided at this point */
		NULL, NULL, 
		/* no close channel provided because it is not
		 * necessary. */
		NULL, NULL,
		/* provide a first level frame receive handler */
		tunnel_frame_received_handler, NULL);

	/* register the especial start extended handler */
	vortex_profiles_register_extended_start (TUNNEL_PROFILE, 
						 tunnel_start_request, NULL);

	return true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void tunnel_close ()
{
	msg ("turbulence TUNNEL close");
}

/**
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-tunnel",
	"BEEP proxy, TUNNEL profile",
	tunnel_init,
	tunnel_close
};

/** 
 * @internal Implementation for the start tunnel request on the
 * connection provided.
 */
bool tunnel_start_request (char             * profile, 
			   int                channel_num, 
			   VortexConnection * connection, 
			   char             * serverName, 
			   char             * profile_content, 
			   char            ** profile_content_reply, 
			   VortexEncoding     encoding, 
			   axlPointer         user_data)
{
	axlDoc   * doc;
	axlError * error;

	msg ("received request to tunnel..\n");
	doc = axl_doc_parse (profile_content, strlen (profile_content), &error);
	if (doc == NULL) {
		/* fill an error to be reported */
		*profile_content_reply = vortex_frame_get_error_message ("500", "failed to parse incoming tunnel spec.", NULL);

		/* free the error */
		axl_error_free (error);

		return false;
	} /* end if */

	msg ("document parsed, resolv tunnel");
	
	/* free document */
	axl_doc_free (doc);

	return true;
}

/** 
 * @internal Implementation for the frame received for this profile. 
 */
void tunnel_frame_received_handler (VortexChannel    * channel, 
				    VortexConnection * connection, 
				    VortexFrame      * frame, 
				    axlPointer         user_data)
{
	
}

END_C_DECLS

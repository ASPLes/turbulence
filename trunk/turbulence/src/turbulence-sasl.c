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
#ifndef __TURBULENCE_SASL_H__
#define __TURBULENCE_SASL_H__

#include <turbulence.h>

bool     turbulence_sasl_plain_validation  (VortexConnection * connection,
					    const char       * auth_id,
					    const char       * authorization_id,
					    const char       * password)
{
	
	/* At this point your server application should connect
	 * to its internal user/password database to validate
	 * incoming request. 
	 *
	 * In this case we perform a validation based on receiving
	 * a pair based on bob/secret allowing it, and denying
	 * the rest of user/password pairs. */
	
        if (!g_strcasecmp (auth_id, "bob") && 
            !g_strcasecmp (password, "secret")) {
		/* notify Vortex that the given SASL request have been
		 * accepted by the application level. */
                return true;
        }

        /* deny SASL request to authenticate remote peer */
        return FALSE;
}

/** 
 * @brief Enable turbulence sasl support.
 * 
 */
void turbulence_sasl_enable ()
{

	msg ("activating SASL support..");
	/* check for SASL support */
	if (!vortex_sasl_is_enabled ()) {
		error ("Unable to start SASL support, vortex library found doesn't have SASL support activated");
		turbulence_exit (-1);
		return;
	} /* end if */

	msg ("ok, vortex library has sasl support");

	/* accept plain profile */
	vortex_sasl_set_plain_validation (turbulence_sasl_plain_validation);
 
	/* accept SASL PLAIN incoming requests */
	if (! vortex_sasl_accept_negociation (VORTEX_SASL_PLAIN)) {
		error ("Unable accept incoming SASL PLAIN profile");
		turbulence_exit (-1);
	} /* end if */
	
	return;
}

/** 
 * @brief Allows to clean up the sasl module.
 */
void turbulence_sasl_cleanup ()
{
	return;
}

#endif

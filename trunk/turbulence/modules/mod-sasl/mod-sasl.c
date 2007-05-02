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
#include <mod-sasl.h>

axlDoc      * sasl_xml_conf    = NULL;
axlDoc      * sasl_xml_db      = NULL;
char        * sasl_xml_db_path = NULL;
VortexMutex   sasl_xml_db_mutex;

bool     mod_sasl_plain_validation  (VortexConnection * connection,
				     const char       * auth_id,
				     const char       * authorization_id,
				     const char       * password)
{
	axlNode     * node;
	const char  * user_id;
	const char  * db_password;

	msg ("required to auth: auth_id=%s, authorization_id=%s", 
	     auth_id ? auth_id : "", authorization_id ? authorization_id : "");

	/* load db */
	if (! common_sasl_load_users_db (&sasl_xml_db, sasl_xml_db_path, &sasl_xml_db_mutex))
		return false;

	/* look up for the user and its password */
	node = axl_doc_get (sasl_xml_db, "/sasl-auth-db/auth");
	while (node != NULL) {
		
		/* get user id to check */
		user_id = ATTR_VALUE (node, "user_id");
		
		/* check the format for the user stored */
		if (axl_memcmp (user_id, "text:", 5)) {
			/* check the user id */
			if (axl_cmp (auth_id, user_id + 5)) {

				/* user found, check if the account is
				 * disabled */
				if (HAS_ATTR_VALUE (node, "disabled", "yes")) {
					error ("trying to auth an account disabled: %s", auth_id);
					return false;
				}

				/* user id found, check password */
				db_password = ATTR_VALUE (node, "password");
				
				/* check the format for the password
				 * stored */
				if (axl_memcmp (db_password, "text:", 5)) {

					/* return if both passwords
					 * are equal */
					return axl_cmp (password, db_password + 5);

				} /* end if */

			} /* end if */
		} /* end if */
		
		/* get next node */
		node = axl_node_get_next (node);
	} /* end if */

        /* deny SASL request to authenticate remote peer */
	error ("auth failed for auth_id=%s", auth_id);
        return false;
}

/** 
 * @brief Loads current sasl configuration and user databases.
 */
bool sasl_load_config ()
{
	/* load and check sasl conf */
	if (! common_sasl_load_config (&sasl_xml_conf, 
				       /* xml backend */
				       &sasl_xml_db_path, &sasl_xml_db, &sasl_xml_db_mutex)) {
		/* failed to load sasl module */
		return false;
	}

	/* sasl loaded and prepared */
	return true;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other task. The function must
 * return true to signal that the module was initialized
 * ok. Otherwise, false must be returned.
 */
static bool sasl_init ()
{
	msg ("turbulence SASL init");
	axlNode * node;

	/* check for SASL support */
	if (!vortex_sasl_is_enabled ()) {
		error ("Unable to start SASL support, vortex library found doesn't have SASL support activated");
		turbulence_exit (-1);
		return false;
	} /* end if */

	/* init mutex */
	vortex_mutex_create (&sasl_xml_db_mutex);

	/* load configuration file */
	if (! sasl_load_config ())
		return false;

	/* check for sasl methods to be activated */
	node = axl_doc_get (sasl_xml_conf, "/mod-sasl/method-allowed/method");
	while (node != NULL) {

		/* check for plain profile */
		if (HAS_ATTR_VALUE (node, "value", "plain")) {
			/* accept plain profile */
			vortex_sasl_set_plain_validation (mod_sasl_plain_validation);
			
			/* accept SASL PLAIN incoming requests */
			if (! vortex_sasl_accept_negociation (VORTEX_SASL_PLAIN)) {
				error ("Unable accept incoming SASL PLAIN profile");
			} /* end if */			
		} /* end if */
		
		/* get the next node */
		node = axl_node_get_next (node);

	} /* end if */

	return true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void sasl_close ()
{
	msg ("turbulence SASL close");
	axl_doc_free (sasl_xml_db);
	axl_doc_free (sasl_xml_conf);
	axl_free     (sasl_xml_db_path);

	/* close mutex */
	vortex_mutex_destroy (&sasl_xml_db_mutex);
}

/**
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-sasl",
	"Auth functions, SASL profile",
	sasl_init,
	sasl_close
};


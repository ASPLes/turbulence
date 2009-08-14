/*
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
#include <mod-sasl.h>
#include <service_dispatch.h>

SaslAuthBackend * sasl_backend     = NULL;
char            * sasl_xml_db_path = NULL;
VortexMutex       sasl_xml_db_mutex;
TurbulenceCtx   * ctx              = NULL;

int      mod_sasl_plain_validation  (VortexConnection * connection,
				     const char       * auth_id,
				     const char       * authorization_id,
				     const char       * password)
{
	msg ("required to auth: auth_id=%s, authorization_id=%s", 
	     auth_id ? auth_id : "", authorization_id ? authorization_id : "");

	/* call to authenticate */
	if (common_sasl_auth_user (sasl_backend, 
				   connection,
				   auth_id, 
				   authorization_id, 
				   password,
				   vortex_connection_get_server_name (connection),
				   &sasl_xml_db_mutex)) {
		return true;
	} /* end if */

        /* deny SASL request to authenticate remote peer */
	error ("auth failed for auth_id=%s", auth_id);
        return false;
}

/** 
 * @brief Loads current sasl configuration and user databases.
 */
int  sasl_load_config (TurbulenceCtx * ctx)
{
	/* load and check sasl conf */
	if (! common_sasl_load_config (ctx, &sasl_backend, NULL, &sasl_xml_db_mutex)) {
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
static int  sasl_init (TurbulenceCtx * _ctx)
{
	int  at_least_one_method = false;

	/* prepare mod-sasl module */
	TBC_MOD_PREPARE (_ctx);

	msg ("turbulence SASL init");

	/* check for SASL support */
	if (!vortex_sasl_init (TBC_VORTEX_CTX(_ctx))) {
		error ("Unable to start SASL support, init function failed");
		/* call to check clean start */
		CLEAN_START(ctx);
		return false;
	} /* end if */

	/* init mutex */
	vortex_mutex_create (&sasl_xml_db_mutex);

	/* load configuration file */
	if (! sasl_load_config (ctx)) {
		/* call to check clean start */
		CLEAN_START(ctx);

		return false;
	}

	/* check for sasl methods to be activated */
	if (common_sasl_method_allowed (sasl_backend, "plain", &sasl_xml_db_mutex)) {
		
		msg ("configuring PLAIN authentication method..");
		/* accept plain profile */
		vortex_sasl_set_plain_validation (TBC_VORTEX_CTX(ctx), mod_sasl_plain_validation);
		
		/* accept SASL PLAIN incoming requests */
		if (! vortex_sasl_accept_negotiation (TBC_VORTEX_CTX(ctx), VORTEX_SASL_PLAIN)) {
			error ("Unable accept incoming SASL PLAIN profile");
		} /* end if */			
		
		/* set that at least one method is activated */
		at_least_one_method = true;
	} else {
		error ("not allowed PLAIN authentication method..");
		
	} /* end if */

	/* check databases that have remote admin */
	if (common_sasl_activate_remote_admin (sasl_backend, &sasl_xml_db_mutex)) {
		/* install the xml-rpc profile support to handle session share
		 * services */
		vortex_xml_rpc_accept_negotiation (
			/* vortex context */
			TBC_VORTEX_CTX(ctx), 
			/* no resource validation function */
			common_sasl_validate_resource,
			/* no user space data for the validation resource
			 * function. */
			sasl_backend,
			service_dispatch,
			/* no user space data for the dispatch function. */
			sasl_backend);
	}

	/* return if one authentication method as activated */
	return at_least_one_method;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void sasl_close (TurbulenceCtx * ctx)
{
	msg ("turbulence SASL close");
	/* call to free all resources dumping back to disk current
	 * state */
	common_sasl_free (sasl_backend);
	
	/* close mutex */
	vortex_mutex_destroy (&sasl_xml_db_mutex);

	return;
}

/** 
 * @brief This handler is called when the module is being unloaded
 * because configuration have signaled the module must not be
 * available at at child process. In many cases this function can be
 * implemented by doing a call to current close handler. 
 * 
 * However, there are some cases that would be required to uninstall
 * all memory and elements used keeping in mind the module may be
 * still in by turbulence main process.
 */
static void sasl_unload (TurbulenceCtx * ctx)
{
	msg ("unloading SASL module..");

	/* call to finish memory without dumping */
	common_sasl_free_common (sasl_backend, axl_false);

	/* close mutex */
	vortex_mutex_destroy (&sasl_xml_db_mutex);

	return;
}

/**
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-sasl",
	"Auth functions, SASL profile",
	sasl_init,
	sasl_close,
	/* no reconf function for now */
	NULL,
	sasl_unload
};


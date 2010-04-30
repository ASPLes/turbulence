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
#include <mod-sasl.h>
#include <service_dispatch.h>

SaslAuthBackend * sasl_backend     = NULL;
VortexMutex       sasl_db_mutex;
VortexMutex       sasl_top_mutex;
TurbulenceCtx   * ctx              = NULL;

axlPointer      mod_sasl_validation  (VortexConnection * connection,
				      VortexSaslProps  * props,
				      axlPointer         user_data)
{
	const char * serverName = vortex_connection_get_server_name (connection);

	if (serverName == NULL)
		serverName = props->serverName;

	msg ("required to auth: auth_id=%s, authorization_id=%s, serverName=%s", 
	     props->auth_id ? props->auth_id : "", props->authorization_id ? props->authorization_id : "", serverName ? serverName : "");

	/* call to authenticate */
	if (axl_cmp (props->mech, VORTEX_SASL_PLAIN)) {
		if (common_sasl_auth_user (sasl_backend, 
					   connection,
					   props->auth_id, 
					   props->authorization_id, 
					   props->password,
					   serverName,
					   &sasl_db_mutex)) {
			return INT_TO_PTR (axl_true);
		} /* end if */
	} /* end if */

        /* deny SASL request to authenticate remote peer */
	error ("auth failed for auth_id=%s", props->auth_id);
        return INT_TO_PTR (axl_false);
}


/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other task. The function must
 * return axl_true to signal that the module was initialized
 * ok. Otherwise, axl_false must be returned.
 */
static int  mod_sasl_init (TurbulenceCtx * _ctx)
{
	/* prepare mod-sasl module */
	TBC_MOD_PREPARE (_ctx);

	msg ("turbulence SASL init");

	/* check for SASL support */
	if (!vortex_sasl_init (TBC_VORTEX_CTX(_ctx))) {
		error ("Unable to start SASL support, init function failed");
		/* call to check clean start */
		CLEAN_START(ctx);
		return axl_false;
	} /* end if */


	/* initialize backends */
	

	/* init mutex */
	vortex_mutex_create (&sasl_db_mutex);
	vortex_mutex_create (&sasl_top_mutex);

	return axl_true;
}

/** 
 * @brief Handler called once a profile path was selected for a
 * particular connection.
 */
static axl_bool mod_sasl_ppath_selected (TurbulenceCtx      * ctx, 
					 TurbulencePPathDef * ppath_selected, 
					 VortexConnection   * conn) {

	const char * serverName;
	const char * workDir;
	axl_bool     result;

	serverName = turbulence_ppath_get_server_name (conn);
	workDir    = turbulence_ppath_get_work_dir (ctx, ppath_selected);

	msg ("(mod-sasl) notified profile path selected, ppath serverName: '%s' (conn id=%d), workdir: %s", 
	     serverName ? serverName : "", 
	     vortex_connection_get_id (conn),
	     workDir ? workDir : "");

	/* check if the database was already loaded */
	vortex_mutex_lock (&sasl_top_mutex);
	if (sasl_backend != NULL) {
		/* check for default database */
		if (serverName == NULL) {
			result = common_sasl_has_default (sasl_backend, NULL, &sasl_db_mutex);
			if (! result) 
				wrn ("Found no serverName defined and there is no default SASL database for current backend loaded..");
			vortex_mutex_unlock (&sasl_top_mutex);
			return axl_true;
		} /* end if */

		/* ok, database already loaded, check if it supports
		   the serverName requested or the default */
		if (common_sasl_serverName_exists (sasl_backend, serverName, NULL, &sasl_db_mutex)) {
			msg ("SASL database for serverName=%s already loaded", serverName);
			vortex_mutex_unlock (&sasl_top_mutex);
			return axl_true;
		} /* end if */

		/* try to load database associated to serverName */
		result = common_sasl_load_serverName (ctx, sasl_backend, serverName, &sasl_db_mutex);
		msg ("SASL database load for serverName=%s status=%d", serverName, result);
		vortex_mutex_unlock (&sasl_top_mutex);
		return axl_true;
	}

	/* load configuration file and populate backend with the
	   serverName required for this connection */
	if (! common_sasl_load_config (ctx, &sasl_backend, workDir, serverName, &sasl_db_mutex))  {
		wrn ("Failed to load SASL configuration for ppath selected '%s' and connection id %d", 
		     turbulence_ppath_get_name (ppath_selected), vortex_connection_get_id (conn));
		vortex_mutex_unlock (&sasl_top_mutex);
		return axl_false;
	} /* end if */

	/* check for sasl methods to be activated */
	if (common_sasl_method_allowed (sasl_backend, "plain", &sasl_db_mutex)) {
		
		msg ("configuring PLAIN authentication method..");
		/* accept plain profile */
		if (! vortex_sasl_accept_negotiation_common (TBC_VORTEX_CTX (ctx), VORTEX_SASL_PLAIN, mod_sasl_validation, NULL)) {
			error ("Unable accept incoming SASL PLAIN profile");
		} /* end if */			
	} else {
		error ("not allowed PLAIN authentication method..");
	} /* end if */

	/* check databases that have remote admin */
	if (common_sasl_activate_remote_admin (sasl_backend, &sasl_db_mutex)) {
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

	vortex_mutex_unlock (&sasl_top_mutex);
	return axl_true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_sasl_close (TurbulenceCtx * ctx)
{
	msg ("turbulence SASL close");
	/* call to free all resources dumping back to disk current
	 * state */
	common_sasl_free (sasl_backend);
	
	/* close mutex */
	vortex_mutex_destroy (&sasl_db_mutex);
	vortex_mutex_destroy (&sasl_top_mutex);

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
static void mod_sasl_unload (TurbulenceCtx * ctx)
{
	msg ("unloading SASL module..");

	/* call to finish memory without dumping */
	common_sasl_free_common (sasl_backend, axl_false);

	/* close mutex */
	vortex_mutex_destroy (&sasl_db_mutex);
	vortex_mutex_destroy (&sasl_top_mutex);

	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-sasl",
	"Auth functions, SASL profile",
	mod_sasl_init,
	mod_sasl_close,
	/* no reconf function for now */
	NULL,
	mod_sasl_unload,
	/* profile path selected */
	mod_sasl_ppath_selected
};

/** 
 * \page turbulence_mod_sasl mod-sasl: SASL support for Turbulence
 *
 * \section turbulence_mod_sasl_intro Introduction
 *
 * <b>mod-sasl</b> module provides user authentication to Turbulence. Inside
 * BEEP, the SASL protocol is used by default to provide user
 * authentication.
 *
 * \section turbulence_mod_sasl_configuration Base configuration
 *
 * <b>mod-sasl</b> is a module that provides SASL support for turbulence. It
 * includes facilities to configure which SASL profiles can be enabled
 * and the users database to be used.
 *
 * This module is included in the Turbulence official distribution. To
 * enable it you must make it available at some of the directories
 * that are used by Turbulence to load modules (see \ref
 * turbulence_modules_configuration). Under most cases this is done as
 * follows:
 *
 * \code
 *   >> cd /etc/turbulence/mod-enabled
 *   >> ln -s ../mod-available/mod-sasl.xml
 * \endcode
 *
 *
 * Once included the module you must restart Turbulence. Now the
 * <b>mod-sasl</b> is activated you must configure it. This is done by
 * updating <b>sasl.conf</b> file which is usually located at
 * <b>/etc/turbulence/sasl/sasl.conf</b>. Here is an example:
 * 
 * \htmlinclude sasl.example.conf.tmp
 *
 * Previous configuration file example states that there is a default
 * authentication database (no <b>serverName</b> configured), using the md5
 * format to store passwords (<b>format</b>), storing such user and password
 * using the default xml backend provided by turbulence (<b>type</b>), which
 * is located at the file provided (<b>location</b>).
 *
 * The two remaining parameters (<b>remote-admins</b> and <b>remote</b>) allows to
 * configure the remote <b>mod-sasl</b> xml-rpc based administration
 * interface and to configure the set of allowed users that could use
 * this interface. See later for more details.
 *
 * The rest of the file configures the allowed SASL profiles to be
 * used by remote peers. Currently we only support plain.  Virtual
 * host configuration for SASL module
 *
 * Previous example shows how to configure the default backend used
 * for all <b>serverName</b> configurations. Inside BEEP, once a channel is
 * created, it is allowed to configure the <b>serverName</b> parameter asking
 * the server to act using such role. This value can be used to select
 * the proper auth backend configuration.
 *
 * How the <b>mod-sasl</b> selects the proper auth-db is done as follows:
 *
 * <ol>
 *
 *  <li>If the SASL request is being received in a connection which
 *  has the <b>serverName</b> parameter configured (either due to a previous
 *  channel created or due to the current SASL channel exchange), then
 *  it is searched a <b><auth-db></b> node with matches the <b>serverName</b>
 *  parameter.</li>
 *
 *  <li>If no match is found in the previous search, it is used the
 *  first <b><auth-db></b> node found without the <b>serverName</b>
 *  attribute configured. That is, the <b><auth-db></b> node
 *  configured without <b>serverName</b> is used as fallback default
 *  auth-db for all auth operations.</li>
 *
 * </ol>
 *
 * \section turbulence_mod_sasl_cli Command line interface to the mod-sasl: tbc-sasl-conf
 *
 * If you have a shell account into the turbulence machine, you can
 * use the <b>tbc-sasl-conf</b> tool to update users database (<b>auth-db.xml</b>),
 * rather than editing directly. You can add a user using the
 * following:
 *
 * \code
 *   >> tbc-sasl-conf --add-user beep-test
 *   I: adding user: beep-test..
 *   Password:
 *   Type again:
 *   I: user beep-test added!
 * \endcode
 *
 *
 * You can use the --serverName option to select the auth-db to be
 * used by the tool.
 *
 * \code
 *   >> tbc-sasl-conf --add-user beep-test --serverName beep.aspl.es
 *   I: adding user: beep-test..
 *   Password:
 *   Type again:
 *   I: user beep-test added!
 * \endcode
 *
 * Use <b>tbc-sasl-conf --help</b> to get full help.
 *
 * \section turbulence_mod_sasl_ramdin SASL-RADMIN: xml-rpc interface to manage SASL databases
 *
 * Starting from Turbulence 0.3.0, it is included a xml-rpc interface
 * that allows full management for all sasl databases installed. This
 * interface is mainly provided as a way to integrate into third party
 * applications the possibility to manage users, passwords, etc for
 * applications developed.
 * 
 * The following are a minimal set of instructions that will serve you
 * as starting point to integrate and use SASL-RADMIN.
 *
 * <ol>
 *
 *   <li>First you must locate the idl interface installed in your
 *   system. This is the sasl-radmin.idl file. On systems where
 *   pkg-config is available you can find it by using the following:
 *
 * \code
 *   >> pkg-config --cflags sasl-radmin
 *   /usr/share/mod-sasl/radmin/sasl-radmin.idl
 * \endcode
 *
 *  </li>
 *
 *  <li>This idl file includes not only the interface definition but
 *  also the code for all services provided. To build the C client
 *  interface, so you can perform XML-RPC invocations, you can do:
 *
 * \code
 *  >> xml-rpc-gen --out-stub-dir . --only-client --disable-main-file
 *                 --disable-autoconf /usr/share/mod-sasl/radmin/sasl-radmin.idl
 * \encode
 *
 * In the case your system has pkg-config, you can do the following:
 *
 * \code
 *  >> xml-rpc-gen --out-stub-dir . --only-client --disable-main-file
 *                   --disable-autoconf `pkg-config --cflags sasl-radmin`
 *  [ ok ] compiling: /usr/share/mod-sasl/radmin/sasl-radmin.idl..
 *  [ ok ] detected IDL format definition..
 *  [ ok ] detected xml-rpc definition: 'sasl-radmin'..
 *  [ ok ] found enforced resource: sasl-radmin
 *  [ ok ] registered valued attribute resource='sasl-radmin'..
 *  [ ok ] service declaration get_users found..
 *  [ ok ]   found additional options for get_users
 *  [ ok ]   found include on body declaration for get_users
 *  [ ok ]   found include on body file: get-users-include.c
 *  [ EE ] Failed to open file: get-users-include.c
 *  [ ok ] registered valued attribute resource='sasl-radmin'..
 *  [ ok ] service declaration operate_sasl_user found..
 *  [ ok ]   found additional options for operate_sasl_user
 *  [ ok ]   found include on body declaration for operate_sasl_user
 *  [ ok ]   found include on body file: operate-sasl-user.c
 *  [ EE ] Failed to open file: operate-sasl-user.c
 *  [ ok ] document is well-formed: /usr/share/mod-sasl/radmin/sasl-radmin.idl..
 *  [ ok ] document is valid: /usr/share/mod-sasl/radmin/sasl-radmin.idl..
 *  [ ok ] component name: 'sasl-radmin'..
 *  [ ok ] using '.' as out directory..
 *  [ ok ] generating client stub at: ...
 *  [ ok ] creating file:             ./sasl_radmin_xml_rpc.h
 *  [ ok ] creating file:             ./sasl_radmin_struct_sasluser_xml_rpc.h
 *  [ ok ] creating file:             ./sasl_radmin_struct_sasluser_xml_rpc.c
 *  [ ok ] creating file:             ./sasl_radmin_array_sasluserarray_xml_rpc.h
 *  [ ok ] creating file:             ./sasl_radmin_array_sasluserarray_xml_rpc.c
 *  [ ok ] creating file:             ./sasl_radmin_types.h
 *  [ ok ] creating file:             ./sasl_radmin_xml_rpc.c
 *  [ ok ] server stub have been disabled..
 *  [ ok ] compilation ok
 * \endcode
 *
 * Don't mind about EE messages about missing files. Those ones are
 * used by the server side component, which is not your case. Along
 * with the output, the command list the set of files installed.
 * </li>
 *
 * <li>This will build a set of files that must be integrated into
 * your client application according to your building tools. For
 * autoconf users just include that files into your <b>"product_SOURCES"</b>
 * directive. See Vortex xml-rpc-gen tool documentation for more
 * information for an initial explanation.</li>
 * 
 * <li>As a last step, you must activate the remote administration on
 * each domain that will be allowed to do so. This done by using the
 * <b>remote</b> and <b>remote-admins</b> attributes. The first one must be set to
 * <b>"yes"</b>. The second must point to a db-list having a list of allowed
 * SASL administrators. See \ref turbulence_db_list_management to setup the
 * administrator list.</li>
 * </ol>
 */

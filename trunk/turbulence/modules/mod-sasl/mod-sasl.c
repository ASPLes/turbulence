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

axl_bool mod_sasl_load_extension_modules (TurbulenceCtx * ctx)
{
	axlDoc           * modules;
	axlNode          * node;
	axlError         * error = NULL;
	char             * path;

	/* find extension file on configured sasl domain */
	path  = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "sasl", "extension.modules");
	if (path == NULL)  {
		/* not found, load from default location */
		path  = vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "sasl", "extension.modules", NULL);
	} /* end if */

	if (! vortex_support_file_test (path, FILE_EXISTS)) {
		msg ("No SASL extension modules found at %s, skipping..", path);
		axl_free (path);
		return axl_true;
	} /* end if */

	/* open extension modules */
	modules = axl_doc_parse_from_file (path, &error);
	if (modules == NULL) {
		error ("Failed to open SASL extension modules %s, error was: %s", path, axl_error_get (error));
		axl_free (path);
		axl_error_free (error);
		return axl_true;
	} /* end if */

	/* now iterate over all registered modules calling to
	   initialize it */
	node = axl_doc_get (modules, "/extensions/ext");
	while (node) {
		/* call to load module if possible */
		turbulence_module_open_and_register (ctx, ATTR_VALUE (node, "location"));

		/* get next extension */
		node = axl_node_get_next_called (node, "ext");
	} /* end while */

	/* free resources */
	axl_doc_free (modules);
	axl_free (path);

	return axl_true;
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
		return axl_false;
	} /* end if */


	/* initialize backends */
	if (! mod_sasl_load_extension_modules (ctx)) {
		error ("Unable to load SASL extension modules, init function failed");
		return axl_false;
	} /* end if */

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

	msg ("(mod-sasl) notified profile path selected, ppath serverName=%s (conn id=%d), workdir: %s", 
	     serverName ? serverName : "", 
	     vortex_connection_get_id (conn),
	     workDir ? workDir : "");

	/* check if the database was already loaded */
	vortex_mutex_lock (&sasl_top_mutex);
	if (sasl_backend != NULL) {
		msg ("sasl_backend defined, trying to find or load database");
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

	/* check if the sasl_backend is null to return now. When
	 * sasl_backend is null and common_sasl_load_config returns
	 * axl_true means that all SASL databases are right but no
	 * database can server the serverName requested. To avoid
	 * returning axl_false, causing this profile path to be not
	 * accepted, we stop here and return axl_true. No SASL service
	 * will be available but the profile path will function. */
	if (sasl_backend == NULL) {
		vortex_mutex_unlock (&sasl_top_mutex);
		return axl_true;
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
 * \section turbulence_mod_sasl_index Index
 *
 * - \ref turbulence_mod_sasl_intro
 * - \ref turbulence_mod_sasl_configuration
 * - \ref turbulence_mod_sasl_work_dir
 * - \ref turbulence_mod_sasl_cli
 * - \ref turbulence_mod_sasl_ramdin
 * - \ref turbulence_mod_sasl_mysql
 * - \ref turbulence_mod_sasl_creating_password
 * - \ref turbulence_mod_sasl_search_path
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
 * \section turbulence_mod_sasl_work_dir Using work-dir attribute to set an user defined SASL database
 *
 * In the case the profile path under which the connection is running
 * have <b>work-dir</b> attribute defined, then mod-sasl will try to
 * load <b>sasl.conf</b> from that work-dir. If not found, system sasl.conf
 * file will be loaded.
 *
 * The idea is to allow a user with access to a working directory (and
 * a configured profile path poiting to it) to manage its own
 * database.
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
 *
 * \section turbulence_mod_sasl_mysql Using MySQL to auth SASL accounts
 *
 * This section assumes you already have installed <b>mod-sasl-mysql</b>
 * either due to source "make install" or because your distribution
 * provides a package to install it. To check it, look if you have a
 * file called <b>extension.modules</b> located at
 * ${sysconfdir}/turbulence/sasl directory. To know your sysconfdir
 * value use:
 *
 * \code
 * >> turbulence --conf-location
 * \endcode
 *
 * Inside that file (<b>exension.modules</b>) you should find a
 * declaration like follows:
 *
 * \htmlinclude extension.example.modules.xml
 *
 * To use a MySQL database to auth SASL accounts you must first
 * prepare your MySQL server with some SQL to store those users
 * accounts and prepare your SQL queries to adapt your particular
 * structure. There is no fixed or expected SQL structure, so you can
 * adapt the following example with no problem.
 *
 * The following is a working example used by Turbulence regression
 * test to check <b>mod-sasl-mysql</b> module. It will work in most
 * situations so you can start with it. Connect to your database and
 * run the following. Rembember to change user and password:
 *
 * \include create-mysql-sasl-database.sql
 *
 * Now you need to enable this database to by used by mod-sasl. This
 * is done by updating your <b>sasl.conf</b> (usually found at
 * /etc/turbulence/sasl/sasl.conf) to have a declaration like follows:
 *
 * \code
 *  <!-- mysql database backend format -->
 *  <auth-db type="mysql" 
 *           location="auth-db.mysql.xml" 
 *           format="md5" />
 * \endcode
 *
 * Note that this <b><auth-db></b> declaration do not provide a
 * <b>serverName</b> value. This means it will be used for all auth
 * requests if no other especific database is found for the provided
 * serverName and no other default database is found prior this
 * one. In the case you want to use MySQL only for a particular
 * serverName, set it.
 *
 * Now you have registered this <b><auth-db></b> database,
 * <b>mod-sasl</b> will redirect requests to
 * <b>mod-sasl-mysql</b>. Fine, but now you need to tell
 * mod-sasl-mysql how to handle requests. To do so, fill the
 * <b>auth-db.mysql.xml</b> file with the following:
 *
 * \htmlinclude auth-db.mysql.example.xml-tmp
 *
 * As you can see the file is self explanatory. There is a declaration
 * to provide connection settings to reach the MySQL server and a
 * declaration with the SQL required to get the password.
 *
 * Reached this point you are now able to SASL auth your users using a
 * MySQL database. However..
 *
 * \section turbulence_mod_sasl_creating_password Format and how to create hashed passwords compatible with mod-sasl
 *
 * ..an important detail is how to generate the format used to store
 * the password.  In the case use "plain" format
 * (<b>format="plain"</b> attribute declaration inside
 * <b><auth-db></b>) then nothing especial is required. In the case
 * you use md5 or sha-1 you must hash the password using the
 * corresponding method, upper case the result, and inverleave a ":"
 * sign each 2 positions. For example:
 *
 * \code
 * Plain password: test
 * MD5:            09:8F:6B:CD:46:21:D3:73:CA:DE:4E:83:26:27:B4:F6
 * SHA-1:          A9:4A:8F:E5:CC:B1:9B:A6:1C:4C:08:73:D3:91:E9:87:98:2F:BB:D3
 * \endcode
 *
 * At the same time, it is also supported all password formats
 * provided by crypt(3) API, which looks like:
 *
 * \code
 * >> python
 * Python 2.6.6 (r266:84292, Dec 26 2010, 22:31:48) 
 * [GCC 4.4.5] on linux2
 * Type "help", "copyright", "credits" or "license" for more information.
 * >>> import crypt
 * >>> crypt.crypt ("my-password", "$1$salt")
 * '$1$salt$2UXoCqj/ltHf23Mz35S8u.'
 * >>> crypt.crypt ("my-password", "$5$salt")
 * '$5$salt$EBjPwW99d3EHw71rYuAncvySWwz9r/4NKupG0DQ8vyC'
 * >>> crypt.crypt ("my-password", "$6$salt")
 * '$6$salt$8y2PAuMRzsJn8tKxsjgEfs6mGY7zT9Fv6wEd3WBAP69p6glnYZeIJn179monOoNtrhRmaJE1glw8zZXYLcznD.'
 * \endcode
 *
 * Along with mod-sasl code is included a python script that can help
 * you generating these passwords and as example of code so your
 * application can generate those passwords too. The application is
 * <b>gen-mod-sasl-pass.py</b>, and its content is:
 *
 * \include gen-mod-sasl-pass.py
 *
 * To generate passwords do the following:
 * 
 * \code
 * >> gen-mod-sasl-pass.py md5 YOUR_PASSWORD
 * \endcode
 *
 * \section turbulence_mod_sasl_search_path Configuring mod-sasl search path
 *
 * Currently, mod-sasl searches for sasl.conf and associated files
 * looking for them in the following other:
 *
 * - First is checked if the user provided a \ref turbulence_profile_path_search "&lt;search> path" declaration using the domain <b>"sasl"</b>. In that case, the path configure is used.
 *
 * - Then is checked if profile path has a <b>work-dir</b> attribute. If so, this is used.
 *
 * - Finally default location is used (in Linux it is /etc/turbulence/sasl). Run turbulence --conf-location to know where is the base directory located by looking where is found the default configuration file.
 *
 */

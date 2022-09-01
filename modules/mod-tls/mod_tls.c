/*  Turbulence BEEP application server
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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

/* mod_tls implementation */
#include <turbulence.h>

/* include support for tls */
#include <vortex_tls.h>
#include <openssl/err.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* reference to the module configuration */
axlDoc * mod_tls_conf = NULL;

/** 
 * @internal Load tls.conf file.
 */
axl_bool mod_tls_load_config (void) {
	char     * config;
	axlError * error = NULL;

	if (mod_tls_conf == NULL) {
		/* tls.conf not loaded */
		vortex_support_add_domain_search_path_ref (TBC_VORTEX_CTX(ctx), axl_strdup ("tls"),
							   vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "tls", NULL));

		/* load configuration file */
		config = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "tls", "tls.conf");
		if (config == NULL) {
			error ("Unable to find tls.conf file under expected locations, failed to activate TLS profile (try checking %s/turbulence/tls/tls.conf)",
			       turbulence_sysconfdir (ctx));
			return axl_false;
		}
		msg ("loading mod-tls config: %s", config);
		mod_tls_conf  = axl_doc_parse_from_file (config, &error);
		axl_free (config);
		if (mod_tls_conf == NULL) {
			error ("failed to load mod-tls configuration file, error found was: %s", 
			       axl_error_get (error));
			axl_error_free (error);
			return axl_false;
		} /* end if */
	} else {
		msg ("mod-tls config already loaded..");
	} /* end if */

	return axl_true;
}

axl_bool mod_tls_accept_query (VortexConnection * conn,
			       const char       * serverName)
{
	/* for now, always accept */
	return axl_true;
}

axlNode * mod_tls_find_certificate_node (VortexConnection * conn, const char * serverName)
{
	axlNode * node = axl_doc_get (mod_tls_conf, "/mod-tls/certificate-select");

	msg ("Finding certificate/key material for serverName=%s conn-id=%d",
	     serverName ? serverName : "N/A", vortex_connection_get_id (conn));

	while (node) {

		/* check serverName value */
		if (HAS_ATTR_VALUE (node, "serverName", "*"))
			return node;

		/* check if it matches the name */
		if (HAS_ATTR_VALUE (node, "serverName", serverName))
			return node;

		/* get next node called "certificate-select" */
		node = axl_node_get_next_called (node, "certificate-select");
	} /* end node */

	if (node == NULL) {
		error ("Unable to find <certificate-select> node declaration that matches serverName=%s on conn-id=%d",
		       serverName ? serverName : "N/A", vortex_connection_get_id (conn));
	} /* end if */

	return node;
}

char * mod_tls_check_file_exists (VortexConnection * conn, axlNode * node, const char * key, const char * name)
{
	char * file;

	/* check if the file is relativate or full path */
	if (! turbulence_file_is_fullpath (ATTR_VALUE (node, key))) {
		file = vortex_support_domain_find_data_file (CONN_CTX (conn), "tls", ATTR_VALUE (node, key));
		if (file == NULL) {
			error ("Unable to find %s file %s under configured directories", name, ATTR_VALUE (node, "cert"));
			return NULL;
		}
	} else {
		/* seems path is absolute, return full path */
		if (! vortex_support_file_test (ATTR_VALUE (node, key), FILE_EXISTS)) {
			error ("File %s do not exists, failed to return %s", ATTR_VALUE (node, key), name);
			return NULL;
		}
		/* file exists, copy it */
		file = axl_strdup (ATTR_VALUE (node, key));
	}	

	msg ("Found %s file %s conn-id=%d", name, file, vortex_connection_get_id (conn));
	return file;
}

char * mod_tls_certificate_handler (VortexConnection * conn,
				    const char       * serverName) {

	/* find the certificate node */
	axlNode * node = mod_tls_find_certificate_node (conn, serverName);
	char    * file;

	/* check if the node is null or it has no "cert" attribute */
	if (node == NULL || ! HAS_ATTR (node, "cert"))
		return NULL;

	/* return a copy to the certificate */
	msg ("Selecting certificate file: %s (conn-id=%d, serverName: %s)",
	     ATTR_VALUE (node, "cert"), vortex_connection_get_id (conn), serverName ? serverName : "none");

	/* check if we have the key preloaded */
	if (axl_node_annotate_get (node, "content-certificate", axl_false)) {
		/* found content, return certificate content instead of the path */
		return axl_strdup (axl_node_annotate_get (node, "content-certificate", axl_false));
	} /* end if */

	/* ensure file exists */
	file = mod_tls_check_file_exists (conn, node, "cert", "certificate");
	
	/* return file found */
	return file;
}

char * mod_tls_private_key_handler (VortexConnection * conn,
				    const char       * serverName) {
	/* find the certificate node */
	axlNode * node = mod_tls_find_certificate_node (conn, serverName);
	char    * file;

	/* check if the node is null or it has no "cert" attribute */
	if (node == NULL || ! HAS_ATTR (node, "key"))
		return NULL;

	/* return a copy to the certificate */
	msg ("Selecting private key file: %s (conn-id=%d, serverName: %s)",
	     ATTR_VALUE (node, "key"), vortex_connection_get_id (conn), serverName ? serverName : "none");

	/* check if we have the key preloaded */
	if (axl_node_annotate_get (node, "content-key", axl_false)) {
		/* found content, return private key content instead of the path */
		return axl_strdup (axl_node_annotate_get (node, "content-key", axl_false));
	} /* end if */

	/* ensure file exists */
	file = mod_tls_check_file_exists (conn, node, "key", "private key");
	
	/* return file found */
	return file;
}

void mod_tls_failure_handler (VortexConnection * conn, const char * error_message, axlPointer _ctx)
{
	char            log_buffer [512];
	unsigned long   err;
	const char    * serverName = vortex_connection_get_server_name (conn);
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = _ctx;
#endif
	axlNode       * node = mod_tls_find_certificate_node (conn, serverName);
	axl_bool        close_on_failure = HAS_ATTR_VALUE (node, "close-on-failure", "yes");

	error ("Found connection id=%d (from %s:%s, serverName: %s, close-on-failure: %d) TLS error: %s", vortex_connection_get_id (conn), 
	       vortex_connection_get_host (conn), vortex_connection_get_port (conn), serverName ? serverName : "", close_on_failure,
	       error_message);
	while ((err = ERR_get_error()) != 0) {
		ERR_error_string_n (err, log_buffer, sizeof (log_buffer));
		error ("tls stack: %s (find reason(code) at openssl/ssl.h)", log_buffer);
	}

	/* if the node is defined, but close failure is false and
	 * close-on-failure is not defined in the xml node, then
	 * assume yes for security reasons. It is better to close the
	 * connection after a TLS connection handling failure to avoid
	 * any possibility of tricking the server */
	if (node && ! close_on_failure && ! HAS_ATTR (node, "close-on-failure")) {
		error ("TLS failure found, assuing close-on-failure='yes' because it wasn't defined");
		close_on_failure = axl_true;
	} /* end if */

	/* fulminate this connection if signaled */
	if (close_on_failure) {
		error ("Shutting down connection id=%d due to TLS failure found", vortex_connection_get_id (conn));
		vortex_connection_shutdown (conn);
	} /* end if */

	return;
}

char   * mod_tls_load_file_content (TurbulenceCtx * ctx, const char * file_path)
{
	char         * buffer;
	FILE         * file;
	int            bytes_read;
	struct stat    buf;

	/* skip silently when nothing is found */
	if (! file_path)
		return NULL;

	/* get file size */
	if (stat (file_path, &buf) != 0) {
		error ("Unable to access %s (errno=%d : %s)", file_path, errno, vortex_errno_get_error (errno));
		return NULL;
	} /* end if */

	/* allocate memory to hold the certificate */
	buffer = axl_new (char, buf.st_size + 1);
	if (buffer == NULL) {
		error ("Unable to allocate memory to hold the buffer for the file content");
		return NULL;
	} /* end if */

	/* open file and close it */
	file = fopen (file_path, "r");
	bytes_read = fread (buffer, 1, buf.st_size, file);
	if (bytes_read != buf.st_size) {
		error ("Expected to read %d bytes from %s but found %d",
		       buf.st_size, file_path, bytes_read);
		fclose (file);
		axl_free (buffer);
		return NULL;
	} /* end if */
	fclose (file);

	return buffer;
}

axl_bool mod_tls_preload_certificates (TurbulenceCtx * ctx)
{
	axlNode * node;
	char    * file_content;
	char    * file;

	if (! turbulence_ctx_is_child (ctx)) {
		/* no preload is needed here, just return */
		return axl_true;
	} /* end if */

	/* try to load configuration file */
	if (! mod_tls_load_config ()) {
		return axl_false;
	} /* end if */

	msg ("TLS: Preload certificate for %s", turbulence_child_get_serverName (ctx));
	node = axl_doc_get (mod_tls_conf, "/mod-tls/certificate-select");
	while (node) {
		/* check if there is a certificate declaration with
		   the provided serverName */
		if (HAS_ATTR_VALUE (node, "serverName", turbulence_child_get_serverName (ctx))) {
			/* get file pointed by the node: CERTIFICATE */
			file         = vortex_support_domain_find_data_file (TBC_VORTEX_CTX (ctx), "tls", ATTR_VALUE (node, "cert"));
			file_content = mod_tls_load_file_content (ctx, file);
			if (file_content) {
				/* annotate and report */
				msg ("TLS: preloaded certificate from %s (%s)", file, turbulence_child_get_serverName (ctx));
				axl_node_annotate_data_full (node, "content-certificate", NULL, file_content, axl_free);
			} /* end if */
			axl_free (file);

			/* get file pointed by the node: PRIVATE KEY */
			file         = vortex_support_domain_find_data_file (TBC_VORTEX_CTX (ctx), "tls", ATTR_VALUE (node, "key"));
			file_content = mod_tls_load_file_content (ctx, file);
			if (file_content) {
				/* annotate and report */
				msg ("TLS: preloaded private key from %s (%s)", file, turbulence_child_get_serverName (ctx));
				axl_node_annotate_data_full (node, "content-key", NULL, file_content, axl_free);
			} /* end if */
			axl_free (file);

			/** SECURITY: only load the certificate for
			    the profile path activated to ensure the
			    user running code on this child turbulence
			    is not able to access to other sites
			    certificates.
			**/
			break;
		} /* end if */


		/* get next node called "certificate-select" */
		node = axl_node_get_next_called (node, "certificate-select");
	} /* end while */

	return axl_true;
}

/* mod_tls init handler */
static axl_bool  mod_tls_init (TurbulenceCtx * _ctx) {

	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* enable TLS support on vortex context associated to
	 * turbulence */
	if (! vortex_tls_init (TBC_VORTEX_CTX (_ctx))) {
		error ("Failed to initialize vortex TLS support, unable to start mod-tls");
		return axl_false;
	} /* end if */

	/* pre-reload certificates to have access to them after
	   uid/gid change */
	if (! mod_tls_preload_certificates (_ctx))
		return axl_false;

	/* enable accepting TLS activation */
	if (! vortex_tls_accept_negotiation (TBC_VORTEX_CTX (_ctx), 
					     mod_tls_accept_query,
					     mod_tls_certificate_handler,
					     mod_tls_private_key_handler)) {
		error ("Failed to enable accepting TLS requests..");
		return axl_false;
	} /* end if */

	/* add profile attr alias to allow detecting the TLS profile
	 * at the profile path configuration even after connection
	 * tuning reset */
	turbulence_ppath_add_profile_attr_alias (ctx, VORTEX_TLS_PROFILE_URI, "tls-fication:status");

	/* configure tls failure handler */
	vortex_tls_set_failure_handler (TBC_VORTEX_CTX (_ctx),
					mod_tls_failure_handler,
					_ctx);

	/* return tls initialization ok */
	return axl_true;
} /* end mod_tls_init */

/* mod_tls close handler */
static void mod_tls_close (TurbulenceCtx * _ctx) {
	/* clean mod tls module */
	vortex_tls_cleanup (TBC_VORTEX_CTX (_ctx));

	/* clear configuration */
	axl_doc_free (mod_tls_conf);

	return;
} /* end mod_tls_close */

/* mod_tls reconf handler */
static void mod_tls_reconf (TurbulenceCtx * _ctx) {
	/* reload configuration file */
	return;
} /* end mod_tls_reconf */

/* mod_tls unload handler */
static void mod_tls_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_tls_unload */

axl_bool mod_tls_post_check (VortexConnection * conn,
			     axlPointer         user_data,
			     axlPointer         _ssl,
			     axlPointer         _ctx)
{
	TurbulencePPathDef   * ppath_def = user_data;

	/* restore turbulence state (profile path selected) */
	msg ("Restoring turbulence profile path (%d:%s) on TLS connection id=%d",
	     turbulence_ppath_get_id (ppath_def), turbulence_ppath_get_name (ppath_def), 
	     vortex_connection_get_id (conn));

	__turbulence_ppath_set_selected (conn, ppath_def);
	return axl_true;
}

/* mod_tls ppath-selected handler */
static axl_bool mod_tls_ppath_selected (TurbulenceCtx      * _ctx, 
					TurbulencePPathDef * ppath_selected, 
					VortexConnection   * conn) {
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = _ctx;
#endif

	/* drop a log message */
	msg ("Received request to activate TLS on connection id=%d", vortex_connection_get_id (conn));

	/* try to load configuration file */
	if (! mod_tls_load_config ()) {
		return axl_false;
	} /* end if */

	/* set a post check on this connection to establish turbulence
	 * profile path state */
	vortex_tls_set_post_check (conn, mod_tls_post_check, ppath_selected);
	
	/* always accept */
	return axl_true;
} /* end mod_tls_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_tls",
	"TLS profile module for Turbulence",
	mod_tls_init,
	mod_tls_close,
	mod_tls_reconf,
	mod_tls_unload,
	mod_tls_ppath_selected
};

END_C_DECLS

/** 
 * \page turbulence_mod_tls mod-tls: TLS support for Turbulence
 *
 * \section turbulence_mod_tls_index Index
 *
 * - \ref turbulence_mod_tls_intro
 * - \ref turbulence_mod_tls_configuration
 * - \ref turbulence_mod_tls_paths
 * - \ref turbulence_mod_tls_profile_path
 *
 * \section turbulence_mod_tls_intro Introduction
 *
 * <b>mod-tls</b> module provides facilities to use TLS profile,
 * allowing to secure connection avoiding sending data in plain text.
 *
 * \section turbulence_mod_tls_configuration Base configuration
 *
 * This module is included in the Turbulence official distribution. To
 * enable it you must make it available at some of the directories
 * that are used by Turbulence to load modules (see \ref
 * turbulence_modules_configuration). Under most cases this is done as
 * follows:
 *
 * \code
 *   >> cd /etc/turbulence/mod-enabled
 *   >> ln -s ../mod-available/mod-tls.xml
 * \endcode
 *
 * Once included the module you must restart Turbulence. Now the
 * <b>mod-tls</b> is activated you must configure it. This is done by
 * updating <b>tls.conf</b> file which is usually located at
 * <b>/etc/turbulence/tls/tls.conf</b>. Here is an example:
 * 
 * \htmlinclude tls.example.conf.tmp
 *
 *
 * <b>mod-tls</b> function is simple, you place once xml stanza
 * (<b>certificate-select</b>) for each certificate you declare
 * associated to a particular serverName.  
 *
 * Once a connection is received requesting that serverName, the
 * provided certificate and key are used. You can place as many as you
 * need <b>certificate-select</b> knowing that they are matched from
 * top to bottom. If twice serverName is provided, the first found
 * will take precedence.
 *
 * You can also place a <b>certificate-select</b> stanza with
 * <b>serverName="*"</b> which means that, if no previous stanza
 * matches the requested serverName, then this will be used. 
 *
 * Here is an example to create a private key and an unsigned public
 * certificate associated:
 * \code
 * >> openssl genrsa 1024 > test-private.key
 * >> openssl req -new -x509 -nodes -sha1 -days 3650 -key test-private.key > test-certificate.crt
 * \endcode
 *
 * \section turbulence_mod_tls_paths How paths are handled to find certificates and private keys provided
 *
 * The safest way to provide the right location to your certificates
 * is to provide a full path. In the case full path is not provided,
 * then turbulence will try to locate the file at known tls locations
 * (for example /etc/turbulence/tls).
 *
 * \section turbulence_mod_tls_profile_path How to configure profile path to enable TLS
 *
 * Now you got mod-tls installed and configured, you need to tell
 * turbulence to use TLS profile. A typical escenario is to first
 * enable TLS to protect your connection from third parties and then
 * allow your profiles. Here is an example to be placed inside
 * <b>profile-path-configuration</b> node:
 *
 * \htmlinclude tls-example.xml-tmp
 * 
 * In previous example first we "require" to enable TLS to serve any
 * profile. Only when TLS is properly enabled, the custom profile
 * inside it is allowed.
 * 
 * <b>SECURITY CLARIFICATION:</b> Due the way TLS profile is designed,
 * it is not required to place "connmarks" requiring some value stored
 * on the connection to flag that the TLS session is really
 * established (like it happens with SASL). Here you only place the
 * TLS profile uri, as showed in the example, to protect profiles
 * running inside. This is because the module installs an internal
 * alias value to check the TLS profile successful activation and that value is only
 * available when the connection is completed. Moreover, mod-tls closes the connection if a failure is found to avoid any confusion or implementation failure that may let the channel open (the TLS one) causing the profile path to accept the inner profiles (which could be a problem). That's why it is recommended the close-on-failure="yes" declaration (or not place any declaration about this which in turn will cause the same effect). 
 *
 * Another typical escenario is to first secure the conection with
 * TLS, then require proper SASL authentication, and the provide
 * application profiles. This is done like follows:
 *
 * \htmlinclude tls-sasl-example.xml-tmp
 *
 * \section turbulence_mod_tls_search_path Configuring mod-tls search path
 *
 * Currently, mod-tls searches for tls.conf looking for it in the following other:
 *
 * - First is checked, if the user provided a \ref turbulence_profile_path_search "&lt;search> path" declaration using the domain <b>"tls"</b>. In that case, the path configure is used.
 *
 * - Then is checked if profile path has a <b>work-dir</b> attribute. If so, this is used.
 *
 * - Finally default location is used (in Linux it is /etc/turbulence/tls). Run turbulence --conf-location to know where is the base directory located by looking where is found the default configuration file.
 */

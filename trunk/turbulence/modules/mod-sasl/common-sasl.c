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
#include <common-sasl.h>

/* include dtd definition */
#include <common.sasl.dtd.h>

#define LOCK   vortex_mutex_lock(mutex)
#define UNLOCK vortex_mutex_unlock(mutex)

/** 
 * @internal Type to represent the set of backends that we support to
 * store users databases. 
 */
typedef enum {
	/** 
	 * @internal Database handled by xml files.
	 */
	SASL_BACKEND_XML = 1,
	/** 
	 * @internal Database handled by an external function that
	 * bridges requests into the appropriate database.
	 */
	SASL_BACKEND_FORMAT_HANDLER = 2
} SaslBackEndType;

/** 
 * @internal Type to represent the kind of encoding used to store
 * password in the database.
 */
typedef enum {
	SASL_STORAGE_FORMAT_PLAIN,
	SASL_STORAGE_FORMAT_MD5,
	SASL_STORAGE_FORMAT_SHA1
} SaslStorageFormat;

/** 
 * @internal Type used to represent one connected users
 * database. mod-sasl allows to configure several users databases that
 * are configured for each serverName allowed by the application.
 */
struct _SaslAuthDb {
	/** 
	 * @internal Type of the backend.
	 */
	SaslBackEndType     type;
	/** 
	 * @internal Kind of format used to store data.
	 */
	SaslStorageFormat   format;
	/** 
	 * @internal If the remote administration protocol can be
	 * used.
	 */
	axl_bool            remote_admin;
	/** 
	 * @internal Allowed users to use the remote protocol. 
	 */
	TurbulenceDbList  * allowed_admins;
	/** 
	 * @internal serverName under which the database can be
	 * used. This value can be empty, in such case, it is used as
	 * default.
	 */
	char              * serverName;
	/** 
	 * @internal Reference to the db path, that is, the location
	 * in the file disk.
	 */
	char              * db_path;
	/** 
	 * @internal Reference to the database loaded.
	 */
	axlPointer          db;

	/** 
	 * @brief Time record the last modification for the document.
	 */
	long                db_time;

	/** 
	 * @brief Used to signal if the backend db implemenation must
	 * flush its current memory state to disk or only relaease
	 * memory.
	 */
	axl_bool            dump_on_close;

	/** 
	 * @brief Reference to the xml node (<auth-db>) that pointed
	 * to the auth db definition.
	 */
	axlNode           * node;

	/** 
	 * @brief A reference to the format handler (if defined) that
	 * is associated to the type of auth-db that this structure
	 * represents.
	 */
	ModSaslFormatHandler format_handler;
};

/** 
 * @internal Structure used to store all information about databases
 * used to authenticate users. The structure contains all databases
 * indexed by serverName. 
 *
 * This serverName can be used to separate users databases for
 * different applications. 
 * 
 * For example, you can use the serverName "users.turbulence.ws" to
 * identify the set of profiles to be provided for general users and a
 * particular SASL database to authenticate them. 
 * 
 * At the same time you can use "admin.turbulence.ws" to provide a
 * different SASL database with remote administration support to allow
 * a small set of administrators to manage users.
 *
 */
struct _SaslAuthBackend {
	/** 
	 * @brief Reference to the sasl xml conf. The document itself
	 * loaded.
	 */
	axlDoc           * sasl_xml_conf;

	/** 
	 * @brief Reference to the default database to be used if no
	 * serverName configuration is found. It is only allowed to
	 * have only one default database. 
	 */
	SaslAuthDb       * default_db;
	
	/** 
	 * @brief serverName indexes hash with all databases found.
	 */
	axlHash          * dbs;

	/** 
	 * @brief Reference to the list of root managers, that is, the
	 * set of users that are allowed to manage every domain.
	 */
	TurbulenceDbList * rootManagers;

	/** 
	 * @brief Path to the sasl conf file opened.
	 */
	char             * sasl_conf_path;

	/** 
	 * @brief Reference to the turbulence context.
	 */ 
	TurbulenceCtx    * ctx;

	/** 
	 * @brief Max allowed tries before applying action.
	 */
	int                max_allowed_tries;
	const char       * max_allowed_tries_action;

	/** 
	 * @brief Disabled accounts action.
	 */ 
	const char       * accounts_disabled_action;
};

/** 
 * @internal Function used to deallocate a sasl auth db, that is, a
 * particular connected database.
 * 
 * @param db A reference to the database to dealloc.
 */
void common_sasl_db_free (SaslAuthDb * db)
{
	if (db == NULL)
		return;

	/* close allowed domains */
	if (db->dump_on_close) {
		turbulence_db_list_close (db->allowed_admins);
	} else {
		turbulence_db_list_unload (db->allowed_admins);
	}
	axl_free (db->serverName);
	axl_free (db->db_path);

	/* free users backend */
	if (db->type == SASL_BACKEND_XML)
		axl_doc_free ((axlDoc*) db->db);
	
	/* free the node itself */
	axl_free (db);

	return;
}

/** 
 * @brief Allows to get the TurbulenceCtx object associated to the
 * provided sasl backend.
 * 
 * @param backend The SASL backend.
 * 
 * @return A reference to the TurbulenceCtx object or NULL if it
 * fails.
 */
TurbulenceCtx * common_sasl_get_context (SaslAuthBackend * backend)
{
	v_return_val_if_fail (backend, NULL);

	/* return current sasl status */
	return backend->ctx;
}

/** 
 * @brief Allows to register a new database format that will be
 * supported by common sasl (tools and module) that is associated to
 * the provided turbulence context. The handler provided will
 * implement all operations required by common-sasl.
 *
 * @param ctx The turbulence context where the format will be
 * registered. Cannot be NULL.
 *
 * @param format The format that will be registered. Cannot be NULL or empty (length > 0).
 *
 * @param op_handler The handler to install that will manage requests
 * for this new format. If NULL is provided, the format handler will
 * be uninstalled.
 *
 * @return axl_true in the case the format is registered. If a format
 * was previously registered with the same name, it will replaced with
 * the new one. The function returns axl_false in the case ctx or format parameters are NULL.
 */
axl_bool        common_sasl_register_format (TurbulenceCtx        * ctx,
					     const char           * format,
					     ModSaslFormatHandler   op_handler)
{
	char * str_format;
	
	if (ctx == NULL || format == NULL || strlen (format) == 0)
		return axl_false;
	/* build format string */
	str_format = axl_strdup_printf ("common-sasl:format:%s", format);

	/* register */
	turbulence_ctx_set_data_full (ctx, str_format, op_handler, axl_free, NULL);

	/* handler installed */
	return axl_true;
}

/** 
 * @internal Function used to get the handler registered on the
 * provided turbulence context associated to the provided format.
 *
 * @param ctx The turbulence context where the format handler was registered.
 * @param format The format as a key to retrieve the format handler associated.
 *
 * @return NULL if no format handler was registered, otherwise a
 * reference to the format handler is returned.
 */
ModSaslFormatHandler common_sasl_format_get_handler (TurbulenceCtx * ctx,
						     const char    * format)
{
	char                  * str_format;
	ModSaslFormatHandler    op_handler;
	
	if (ctx == NULL || format == NULL || strlen (format) == 0)
		return axl_false;
	/* build format string */
	str_format = axl_strdup_printf ("common-sasl:format:%s", format);

	/* get handler defined */
	op_handler = turbulence_ctx_get_data (ctx, str_format);

	/* free string format */
	axl_free (str_format);
	
	/* axl_true if the handler is defined */
	return op_handler;
}

/** 
 * @internal Allows to check if the provided format has a handler
 * installed on the provided turbulence context.
 *
 * @param ctx The turbulence context where the format handler was registered.
 * @param format The format to check for handler registered.
 *
 * @return axl_true in the case a format handler is registered,
 * otherwise axl_false is returned.
 */
axl_bool        common_sasl_format_registered (TurbulenceCtx  * ctx,
					       const char     * format)
{
	ModSaslFormatHandler    op_handler;
	
	/* get handler defined */
	op_handler = common_sasl_format_get_handler (ctx, format);
	
	/* axl_true if the handler is defined */
	return (op_handler != NULL);
}

/** 
 * @internal Allows to load an auth database represented in the provided
 * xml node (axlNode).
 */
axl_bool        common_sasl_format_load_db    (TurbulenceCtx    * ctx,
					       SaslAuthBackend  * backend,
					       SaslAuthDb       * db,
					       axlNode          * node,
					       VortexMutex      * mutex)
{
	ModSaslFormatHandler    op_handler;
	axlPointer              result;
	axlError              * err = NULL;

	/* flag the type */
	db->type          = SASL_BACKEND_FORMAT_HANDLER;
	
	/* get handler defined */
	op_handler = common_sasl_format_get_handler (ctx, ATTR_VALUE (node, "type"));

	if (op_handler == NULL) {
		error ("failed to load auth-db with format %s, handler is not defined", ATTR_VALUE (node, "type"));
		return axl_false;
	} /* end if */

	/* call to load auth db */
	result = op_handler (ctx, NULL, backend, node, MOD_SASL_OP_TYPE_LOAD_AUTH_DB,
			     /* auth_id, authorization_id, password, serverName, sasl_method, err, mutex */
			     NULL, NULL, NULL, NULL, NULL, &err, mutex);

	/* check error returned */
	if (PTR_TO_INT(result) == 0) {
		error ("failed to load auth-db with format %s, handler reported failure. Error code: %d, report: %s",
		       ATTR_VALUE (node, "type"), axl_error_get_code (err), axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* axl_true if the handler is defined */
	return axl_true;
}

void common_sasl_free_common (SaslAuthBackend * backend, axl_bool dump_content)
{
	axlHashCursor * cursor;
	SaslAuthDb    * db;

	if (backend == NULL)
		return;

	/* not dump_content flag all backends to not dump */
	cursor = axl_hash_cursor_new (backend->dbs);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get the database */
		db = axl_hash_cursor_get_value (cursor);

		/* configure */
		db->dump_on_close = dump_content;

		/* next item to explore */
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* release the path */
	axl_free            (backend->sasl_conf_path);
	axl_doc_free        (backend->sasl_xml_conf);
	axl_hash_free       (backend->dbs);

	/* free default database */
	if (backend->default_db) {
		backend->default_db->dump_on_close = dump_content;
		common_sasl_db_free (backend->default_db);
	} /* end if */

	/* free node itself */
	axl_free            (backend);

	return;
}

void common_sasl_free (SaslAuthBackend * backend)
{
	/* free and dump content back to disk */
	common_sasl_free_common (backend, axl_true);

	return;
}

/** 
 * @internal Function that implements the load of the
 * max-allowed-tries configuration.
 */
int  common_sasl_get_max_allowed_tries (TurbulenceCtx * ctx, SaslAuthBackend * sasl_backend)
{
	axlNode *node;

	/* get node reference */
	node  = axl_doc_get (sasl_backend->sasl_xml_conf, "/mod-sasl/login-options/max-allowed-tries");

	/* now load and check values */
	sasl_backend->max_allowed_tries = (int) vortex_support_strtod (ATTR_VALUE (node, "value"), NULL);
	if (sasl_backend->max_allowed_tries < 0) {
		/* failed to load some database */
		common_sasl_free (sasl_backend);
		error ("max-allowed-tries, found negative value while expecting 0..n range");
		return axl_false;
	}
	sasl_backend->max_allowed_tries_action = ATTR_VALUE (node, "action");

	return axl_true;
}

/** 
 * @internal Function that implements the load of the
 * accounts-disabled configuration.
 */
int  common_sasl_get_accounts_disabled (TurbulenceCtx * ctx, SaslAuthBackend * sasl_backend)
{
	axlNode *node;

	/* get node reference */
	node  = axl_doc_get (sasl_backend->sasl_xml_conf, "/mod-sasl/login-options/accounts-disabled");

	/* now load and check values */
	sasl_backend->accounts_disabled_action = ATTR_VALUE (node, "action");

	return axl_true;
}

/** 
 * @internal Function used to find sasl.conf file when an alternative
 * location is provided.
 *
 * @param ctx The context where the load operation will take place.
 * @param alt_location The alternative location.
 * @param file The base file name to locate.
 *
 * @return A reference to the file path or NULL if it fails to locate
 * it.
 *
 */
char * common_sasl_find_alt_file (TurbulenceCtx * ctx, 
				  const char    * alt_location, 
				  const char    * file)
{
	char * path;

	/* check to find "sasl.conf" file at the alt_location */
	path = vortex_support_build_filename (alt_location, file, NULL);
	if (vortex_support_file_test (path, FILE_EXISTS | FILE_IS_REGULAR)) 
		return path;
	
	axl_free (path);
	/* check if the alt_location is indeed a
	   direct path to sasl.conf */
	if (strlen (alt_location) > strlen (file) && axl_cmp (alt_location + strlen (alt_location) - strlen (file), file)) {
		return axl_strdup (alt_location);
	} /* end if */
	
	/* configure lookup domain for mod sasl settings */
	vortex_support_add_domain_search_path_ref (TBC_VORTEX_CTX(ctx), axl_strdup ("sasl"), axl_strdup (alt_location));

	/* us the alternative location to load the document */
	path  = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "sasl", file);
	if (path == NULL) {
		/* remove here path added for domain search */
		return NULL;
	}
	/* file located */
	return path;
}


/** 
 * @internal Allows to load a single <auth-db> declaration found inside a
 * particular sasl.conf file. 
 *
 */
axl_bool common_sasl_load_single_auth_db (TurbulenceCtx * ctx, 
					  SaslAuthBackend * sasl_backend,
					  axlNode         * node,
					  const char      * alt_location,
					  VortexMutex     * mutex)
{
	SaslAuthDb * db;

	/* check if it is possible to load the database: it is
	   native xml database or format handled by a external
	   format handler */
	if (! (HAS_ATTR_VALUE (node, "type", "xml") && HAS_ATTR (node, "location")) &&
	    ! common_sasl_format_registered (ctx, ATTR_VALUE (node, "type"))) {
		
		/* format not found or unable to handle */
		error ("Found request to load database with unrecognized format %s (location: %s, serverName: %s)",
		       ATTR_VALUE (node, "type") ? ATTR_VALUE (node, "type") : "", 
		       ATTR_VALUE (node, "serverName") ? ATTR_VALUE (node, "serverName") : "");
		/* failed to load database */
		return axl_false;
	} /* end if */

	/* CREATE: create auth db object */
	db = axl_new (SaslAuthDb, 1);

	/* record node that was used to load this authdb */
	db->node = node;

	/* CONFIGURE: configure if this database is flaged
	   with remote administration and the format for
	   stored passwords */
	db->remote_admin = HAS_ATTR_VALUE (node, "remote", "yes");
	msg2 ("sasl remote admin status: %d", db->remote_admin);
	if (HAS_ATTR_VALUE (node, "format", "sha-1"))
		db->format = SASL_STORAGE_FORMAT_SHA1;
	else if (HAS_ATTR_VALUE (node, "format", "md5"))
		db->format = SASL_STORAGE_FORMAT_MD5;
	else if (HAS_ATTR_VALUE (node, "format", "plain"))
		db->format = SASL_STORAGE_FORMAT_PLAIN;
	else {
		wrn ("using as default storage format: md5");
		db->format = SASL_STORAGE_FORMAT_MD5;
	} /* end if */
	
	/* REGISTER: database according to serverName get the serverName value */
	if (HAS_ATTR (node, "serverName") && strlen (ATTR_VALUE (node, "serverName")) > 0) {
		/* check if there are other database
		 * added for the same serverName */
		if (axl_hash_exists (sasl_backend->dbs, (axlPointer) ATTR_VALUE (node, "serverName"))) {
			error ("found a serverName database associated to the current database (serverName configured twice!)");
			return axl_false;
		}
		
		/* fine, add to the database loaded hash */
		axl_hash_insert_full (sasl_backend->dbs, 
				      /* the key to store and its destroy function */
				      axl_strdup (ATTR_VALUE (node, "serverName")), axl_free,
				      /* the value to store and its destroy function */
				      db, (axlDestroyFunc) common_sasl_db_free);
		
	} else {
		/* no serverName found, it seems this
		 * is the default database to be used,
		 * check if there is already configure
		 * a default database */
		if (sasl_backend->default_db != NULL) {
			error ("it was found several default users databases (serverName empty or not found)");
			common_sasl_db_free (db);
			return axl_false;
		} /* end if */
		
		/* configure as the default one */
		sasl_backend->default_db = db;
	} /* end if */
	
	/* now load the particular content of the database (or
	   connect to it) */
	if (HAS_ATTR_VALUE (node, "type", "xml") &&
	    HAS_ATTR (node, "location")) {
		
		/* call to load the database in xml format */
		if (! common_sasl_load_auth_db_xml (sasl_backend, db, node, alt_location, mutex)) {
			
			error ("SASL: failed to load some databases configured");
			return axl_false;
		} /* end if */
		
	} else if (common_sasl_format_registered (ctx, ATTR_VALUE (node, "type"))) {
		msg ("SASL: found registered handler for %s format", ATTR_VALUE (node, "type"));
		
		/* call to load */
		if (! common_sasl_format_load_db (ctx, sasl_backend, db, node, mutex)) {
			error ("SASL: failed to load some databases configured");
			return axl_false;
		} /* end if */
		
		/* configure the format handler here to avoid calling
		   over and over again to this function */
		db->format_handler = common_sasl_format_get_handler (ctx, ATTR_VALUE (node, "type"));
	} /* end if */

	/* format loaded */
	return axl_true;
}

/** 
 * @brief Public mod-sasl APi that allows to load sasl backend from
 * the default file or the one located using alt_location. The
 * function return on success a reference to the sasl backend loaded,
 * with all databases associated. 
 *
 * @param sasl_backend Caller reference to the sasl_backend to be loaded.
 * 
 * @param alt_location An optional value that allows to configure
 * which is the location of the sasl backend.
 *
 * @param serverName Optional parameter that allows signaling what
 * SASL database must be loaded. This will cause the SaslAuthBackend
 * to load a database that either matches serverName or default
 * database is used.
 *
 * @param mutex Optional mutex variable used to lock the
 * implementation to avoid race conditions between threads.
 */
axl_bool  common_sasl_load_config (TurbulenceCtx    * ctx,
				   SaslAuthBackend ** sasl_backend,
				   const char       * alt_location,
				   const char       * serverName,
				   VortexMutex      * mutex)
{
	axlNode         * node;
	axlError        * err;
	SaslAuthBackend * result;
	char            * path;
	axlDtd          * dtd;

	/* do not oper if a null context is received */
	v_return_val_if_fail (ctx, axl_false);

	/* nullify before doing anyting */
	if (sasl_backend)
		*sasl_backend = NULL;

	/* create the sasl back end node */
	result      = axl_new (SaslAuthBackend, 1);
	result->dbs = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/* configure turbulence ctx */
	result->ctx = ctx;

	/* check alternative location */
	if (alt_location == NULL) {
		/* configure lookup domain for mod sasl settings */
		vortex_support_add_domain_search_path_ref (TBC_VORTEX_CTX(ctx), axl_strdup ("sasl"), 
							   vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "sasl", NULL));
		
		/* find and load the file */
		result->sasl_conf_path  = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "sasl", "sasl.conf");
	} else {
		/* check the the alt_location is a directory or a file */
		if (vortex_support_file_test (alt_location, FILE_IS_REGULAR)) {
			/* use file directly */
			result->sasl_conf_path = axl_strdup (alt_location);
		} else {
			/* find sasl.conf path using provided alt location. */
			result->sasl_conf_path  = common_sasl_find_alt_file (ctx, alt_location, "sasl.conf");
		}
	} /* end if */

	/* check if the path provided is valid */
	if (result->sasl_conf_path == NULL) {
		/* failed to load backend */
		path = vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "sasl", "sasl.conf", NULL);
		error ("Unable to find sasl.conf file. A usual location for this file is %s. Check your installation.", path);
		axl_free (path);
                return axl_false;
	}

	/* load the document */
	msg ("loading SASL configuration from: %s", result->sasl_conf_path);
	result->sasl_xml_conf   = axl_doc_parse_from_file (result->sasl_conf_path, &err);

	/* check result */
	if (result->sasl_xml_conf == NULL) {
		/* release sasl_backend */
		common_sasl_free (result);

		error ("failed to init the SASL profile, unable to find configuration file, error: %s",
		       axl_error_get (err));
		axl_error_free (err);
                return axl_false;
	} /* end if */

	/* now validate content found */
        dtd = axl_dtd_parse (COMMON_SASL_DTD, -1, &err);
        if (dtd == NULL) {
		error ("failed to load common.sasl.dtd to check sasl configuration, error: %s",
		       axl_error_get (err));
		axl_error_free (err);
                return axl_false;
	}

	/* perform DTD validation */
        if (! axl_dtd_validate (result->sasl_xml_conf, dtd, &err)) {
		/* free dtd reference */
		axl_dtd_free (dtd);

		/* release sasl_backend */
		common_sasl_free (result);

		error ("failed to validate SASL module configuration, error: %s",
		       axl_error_get (err));
		axl_error_free (err);
                return axl_false;
        } /* end if */

	/* free dtd reference */
	axl_dtd_free (dtd);

	/* now load all users dbs */
	node                = axl_doc_get (result->sasl_xml_conf, "/mod-sasl/auth-db");
	while (node != NULL) {

		/* check for serverName definition */
		if (serverName != NULL && HAS_ATTR (node, "serverName") && ! HAS_ATTR_VALUE (node, "serverName", serverName)) {
			/* get next auth db */
			node = axl_node_get_next_called (node, "auth-db");
			continue;
		} /* end if */

		/* load single db */
		if (! common_sasl_load_single_auth_db (ctx, result, node, alt_location, mutex)) {
			/* failed to load some database */
			common_sasl_free (result);
			return axl_false;
		}

		/* get the next database */
		node = axl_node_get_next_called (node, "auth-db");

	} /* end while */

	/* get options */
	if (! common_sasl_get_max_allowed_tries (ctx, result))
		return axl_false;
	if (! common_sasl_get_accounts_disabled (ctx, result))
		return axl_false;

	/* set the backend loaded to the caller */
	if (sasl_backend)
		*sasl_backend = result;
	else {
		/* weird case where the programmer didn't provide a
		 * reference to the resulting object */
		common_sasl_free (result);
		(*sasl_backend) = NULL;
                return axl_false;

	} /* end if */
	
	/* after returning, check if the have already loaded one
	 * database */
	if (result->default_db == NULL && axl_hash_items (result->dbs) == 0) {
		/* no database or default database was loaded */
		common_sasl_free (result);
		(*sasl_backend) = NULL; /* undef caller reference */

		if (serverName == NULL) {
			error ("No usable auth-db database found for any domain or default domain, unable to start SASL backend.");
			return axl_false;
		} /* end if */

		wrn ("No usable auth-db database found associated to serverName=%s. Unable to start SASL backed.",
		     serverName);
		/* return started */
                return axl_true;
	}

	msg ("SASL configuration %s loaded OK", result->sasl_conf_path);

	return axl_true;
}

/** 
 * @brief Given a loaded SaslAuthBackend, allows to load the provided
 * serverName (if found), adding it to the backend or just returning
 * if it is already loaded.
 *
 * @param ctx The turbulence context where the load operation will take place.
 * @param sasl_backend A SASL backend already initialized.
 *
 * @param serverName The serverName used to locate the database
 * configuration to load or to check if it is already loaded.
 *
 * @param mutex Optional mutex variable used to lock the
 * implementation to avoid race conditions between threads.
 *
 * @return axl_true In the case the serverName load was done (or it
 * was already done), otherwise axl_false is returned.
 */
axl_bool        common_sasl_load_serverName (TurbulenceCtx   * ctx,
					     SaslAuthBackend * sasl_backend,
					     const char      * serverName,
					     VortexMutex     * mutex)
{
	axlNode * node;

	/* check input parameters */
	v_return_val_if_fail (ctx,          axl_false);
	v_return_val_if_fail (sasl_backend, axl_false);
	v_return_val_if_fail (serverName,   axl_false);

	/* check if the database already this database loaded */
	if (common_sasl_serverName_exists (sasl_backend, serverName, NULL, mutex))
		return axl_true;

	/* now load all users dbs */
	node                = axl_doc_get (sasl_backend->sasl_xml_conf, "/mod-sasl/auth-db");
	while (node != NULL) {

		/* check for serverName definition */
		msg ("Checking database serverName=%s with %s", serverName, ATTR_VALUE (node, "serverName"));
		if (! HAS_ATTR (node, "serverName") || ! HAS_ATTR_VALUE (node, "serverName", serverName)) {
			/* get next auth db */
			node = axl_node_get_next_called (node, "auth-db");
			continue;
		} /* end if */

		/* node found! */
		break;

	} /* end while */

	/* check if we did find the node */
	if (node == NULL) {
		error ("Unable to find <auth-db> node declaration for the serverName=%s", serverName);
		return axl_false;
	} /* end if */

	/* load single db and directly return result */
	return common_sasl_load_single_auth_db (ctx, sasl_backend, node, NULL, mutex);
}

/** 
 * @brief Allows to get the path to the file (sasl.conf) that was
 * loaded to initiate the provided sasl auth backend.
 *
 * @param sasl_backend The backend that is required to return the path
 * that was used to load the sasl.conf file that represents this
 * backend.
 *
 * @return A reference to the path or NULL if it fails.
 */
const char   *  common_sasl_get_file_path   (SaslAuthBackend * sasl_backend)
{
	if (sasl_backend == NULL)
		return NULL;

	/* return path */
	return sasl_backend->sasl_conf_path;
}

/** 
 * @internal Function that performs the loading of the auth-db in xml
 * format.
 * 
 * @param sasl_backend The global sasl backend reference.
 *
 * @param node The reference to the <auth-db> node found in the
 * sasl.conf file.
 * 
 * @return axl_true if the auth-db was properly loaded, otherwise axl_false is
 * returned.
 */
axl_bool  common_sasl_load_auth_db_xml (SaslAuthBackend * sasl_backend,
					SaslAuthDb      * db,
					axlNode         * node,
					const char      * alt_location,
					VortexMutex     * mutex)
{
	/* get a reference to the turbulence context */
	TurbulenceCtx * ctx = sasl_backend->ctx;
	axlError      * err  = NULL;
	char          * base_dir;
	char          * path;

	/* create one db */
	db->dump_on_close = axl_true;
	db->type          = SASL_BACKEND_XML;

	/* find the file */
	if (alt_location) {
		if (vortex_support_file_test (alt_location, FILE_IS_DIR))
			path = axl_strdup (alt_location);
		else 
			path = turbulence_base_dir (alt_location);
		db->db_path  = common_sasl_find_alt_file (ctx, path, ATTR_VALUE (node, "location"));
		axl_free (path);
	} else {
		/* use "sasl" context find the file */
		db->db_path  = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "sasl", ATTR_VALUE (node, "location"));
		if (db->db_path == NULL) {
			/* try to find file at the directory where
			   sasl.conf file was loaded */
			base_dir    = turbulence_base_dir (common_sasl_get_file_path (sasl_backend));
			wrn ("Unable to find %s file on default 'sasl' location, checking at %s", ATTR_VALUE (node, "location"), base_dir);
			db->db_path = axl_strdup_printf ("%s%s%s", base_dir, VORTEX_FILE_SEPARATOR, ATTR_VALUE (node, "location"));
			axl_free (base_dir);
			if (! vortex_support_file_test (db->db_path, FILE_IS_REGULAR)) {
				wrn ("File not found at %s", db->db_path);
				axl_free (db->db_path);
				db->db_path = NULL;
			} /* end if */
		} /* end if */
	} /* end if */

	/* check if the path provided is valid */
	if (db->db_path == NULL) {
		/* failed to load backend */
		path = vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "sasl", NULL);
		error ("Unable to find %s file. A usual location for this file is %s. Check your installation.", 
		       ATTR_VALUE (node, "location"), path);
		axl_free (path);

		/* free node created */
		axl_free (db);
		       
		return axl_false;
	}


	/* load db in xml format */
	if (! common_sasl_load_users_db (ctx, db, mutex)) {
		/* failed to load database */
		wrn ("Unable to load database from: %s, this database won't be usable", 
		     db->db_path);
		
		/* free memory allocated */
		axl_free (db->db_path);
		axl_free (db);
	} else {
		msg2 ("sasl auth db: %s", db->db_path);

		/* check remote admins */
		if (db->remote_admin && HAS_ATTR (node, "remote-admins") && strlen (ATTR_VALUE (node, "remote-admins")) > 0) {

			/* load the turbulence db list */
			msg2 ("found remote admins, loading dblist: '%s'", ATTR_VALUE (node, "remote-admins"));
			if (alt_location) {
				base_dir = turbulence_base_dir (alt_location);
				path     = common_sasl_find_alt_file (ctx, base_dir, ATTR_VALUE (node, "remote-admins"));
				axl_free (base_dir);
			} else {
				path     = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "sasl", ATTR_VALUE (node, "remote-admins"));
			} /* end if */

			if (path) {
				db->allowed_admins = turbulence_db_list_open (ctx, &err, path, NULL);
				axl_free (path);
			} /* end if */
			
			if (db->allowed_admins == NULL) {
				error ("unable to load the list of allowed admins, this will disable the remote administration. Error found: %s",
				       axl_error_get (err));
				axl_error_free (err);
				
				/* flag as activated */
				db->remote_admin = axl_false;
			} /* end if */
		}
		
	} /* end if (type == xml) */

	return axl_true;
}

/** 
 * @internal Function that supports the authentication of SASL users
 * provided a SaslAuthDb using xml format.
 * 
 * @param db The reference to the xml database.
 * @param auth_id The user to authenticate.
 * @param authorization_id The authorization to use.
 * @param password The password to be used.
 * 
 * @return 1 if the account was autenticated. 0 if login or password
 * were not found or they are incorrect. -1 in the case the account is
 * disabled.
 */
axl_bool common_sasl_auth_db_xml (TurbulenceCtx   * ctx,
				  SaslAuthDb      * db, 
				  const char      * auth_id, 
				  const char      * authorization_id, 
				  const char      * password)
{
	axlNode     * node;
	axlDoc      * doc;
	const char  * user_id;
	const char  * db_password;

	/* check file modification */
	if (! common_sasl_load_users_db (ctx, db, NULL))
		return axl_false;

	/* look up for the user and its password */
	doc  = (axlDoc *) db->db;
	node = axl_doc_get (doc, "/sasl-auth-db/auth");
	while (node != NULL) {
		
		/* get user id to check */
		user_id = ATTR_VALUE (node, "user_id");
		
		/* check the user id */
		if (axl_cmp (auth_id, user_id)) {

			/* user found, check if the account is
			 * disabled */
			if (HAS_ATTR_VALUE (node, "disabled", "yes")) {
				error ("trying to auth an account disabled: %s", auth_id);
				return -1;
			}
			
			/* user id found, check password */
			db_password = ATTR_VALUE (node, "password");
			
			/* return if both passwords
			 * are equal */
			if (axl_cmp (password, db_password)) {
				return 1;
			} /* end if */
			
		} /* end if */
			
		/* get next node */
		node = axl_node_get_next (node);
	} /* end if */

	return 0;
}

/** 
 * @internal Function that supports the authentication of SASL users
 * provided a SaslAuthDb using xml format.
 * 
 * @param db The reference to the xml database.
 * @param auth_id The user to authenticate.
 * @param authorization_id The authorization to use.
 * @param password The password to be used.
 * 
 * @return 1 if the account was autenticated. 0 if login or password
 * were not found or they are incorrect. -1 in the case the account is
 * disabled.
 */
axl_bool common_sasl_auth_format_handler (TurbulenceCtx    * ctx, 
					  VortexConnection * conn,
					  SaslAuthBackend  * sasl_backend,
					  SaslAuthDb       * db, 
					  const char       * auth_id, 
					  const char       * authorization_id, 
					  const char       * password, 
					  const char       * serverName,
					  VortexMutex      * mutex)
{
	ModSaslFormatHandler    op_handler;
	axlPointer              result;
	axlError              * err = NULL;

	if (ctx == NULL || auth_id == NULL) {
		error ("Failed to request auth to format handler, ctx or auth_id are NULL");
		return axl_false;
	} /* end if */

	/* get handler defined */
	op_handler = db->format_handler;
	if (op_handler == NULL) {
		error ("Failed to request auth to format handler (%s), it is not defined", ATTR_VALUE (db->node, "type"));
		return axl_false;
	}

	/* call to do auth */
	result = op_handler (ctx, conn, sasl_backend, db->node, MOD_SASL_OP_TYPE_AUTH,
			     /* auth_id, authorization_id, password, serverName, sasl_method, err, mutex */
			     auth_id, authorization_id, password, serverName, NULL, &err, mutex);

	/* account authenticated in the case of 1 */
	if (PTR_TO_INT (result) == 1)
		return axl_true;
	/* in the case or 0 (not authenticated) or -1 (disabled) */
	return axl_false;

}

/** 
 * @internal Function that implements the accounts-disabled option
 * from sasl.conf file.
 */
void common_sasl_apply_accounts_disabled (TurbulenceCtx    * ctx, 
					  SaslAuthBackend  * sasl_backend, 
					  VortexConnection * conn)
{
	/* do not operate if no channel is found. */
	if (conn == NULL)
		return;

	/* check for drop option */
	if (axl_cmp (sasl_backend->accounts_disabled_action, "drop")) {
		wrn ("dropping connection id=%d due to disabled sasl module account policy", 
		     vortex_connection_get_id (conn));
		vortex_connection_shutdown (conn);
	} else if (axl_cmp (sasl_backend->accounts_disabled_action, "none")) {
		msg ("found disabled account, doing nothing according to sasl module policy");
	} 

	/* nothing more for now */
	return;
}

/** 
 * @internal Macro to define the key used to access and store maximum
 * allowded SASL login failure tries.
 */
#define COMMON_SASL_MAX_ALLOWED_TRIES "co:sa:ma:al:tr"

/** 
 * @internal Function that apply the action defined for max-allowed-tries if the limit is reached. 
 */
void common_sasl_apply_max_allowed_tries (TurbulenceCtx    * ctx, 
					  SaslAuthBackend  * sasl_backend, 
					  VortexConnection * conn)
{
	int tries;

	/* do not operate if no channel is found */
	if (conn == NULL)
		return;

	/* check if this is disabled */
	if (axl_cmp (sasl_backend->max_allowed_tries_action, "none")) 
		return;
	
	/* get tries */
	tries = PTR_TO_INT (vortex_connection_get_data (conn, COMMON_SASL_MAX_ALLOWED_TRIES));
	tries++;
	vortex_connection_set_data (conn, COMMON_SASL_MAX_ALLOWED_TRIES, INT_TO_PTR(tries));
	
	/* check if maximum tries has been reached */
	if (tries < sasl_backend->max_allowed_tries) 
		return;

	/* apply actions */
	if (axl_cmp (sasl_backend->max_allowed_tries_action, "drop")) {
		wrn ("dropping connection id=%d due to disabled max allowed tries (%d) account policy", 
		     vortex_connection_get_id (conn), tries);
		vortex_connection_shutdown (conn);
		return;
	} /* end if */
		
	return;
}

/** 
 * @brief Public function used by mod-sasl and its tools to perform
 * user validation against the current backend loaded.
 *
 * The function will try to find a database matching the provided
 * serverName or use the default one either because no database
 * associated to the serverName was found or because no serverName
 * value was provided.
 *
 * The rest of values are required for the authentication itself, that
 * is, auth id and passwords are the usual values to perform the
 * login/password authetication. For those SASL protocols that support
 * authorization id (acting on behalf of) it is also required the
 * authorization_id.
 * 
 * @param sasl_backend The sasl backend where the auth operation will
 * be performed.
 *
 * @param channel The channel where the sasl auth operation is taking
 * place.
 *
 * @param auth_id The auth id that is being required to authenticate
 * (the user login).
 *
 * @param authorization_id For protocols that support auth id proxy.
 *
 * @param password The password.
 *
 * @param serverName The server name under which the SASL
 * authentication must take place.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user was authenticated, otherwise axl_false is
 * returned.
 */
axl_bool  common_sasl_auth_user        (SaslAuthBackend  * sasl_backend,
					VortexConnection * conn,
					const char       * auth_id,
					const char       * authorization_id,
					const char       * password,
					const char       * serverName,
					VortexMutex      * mutex)
{
	/* get a reference to the turbulence context */
	TurbulenceCtx * ctx     = NULL;
	SaslAuthDb    * db      = NULL;
	int             release = axl_false;
	int             result  = 0;

	/* no backend, no authentication */
	if (sasl_backend == NULL || sasl_backend->ctx == NULL) {
		error ("no sasl backend was provided, unable to perform SASL authentication.");
		return axl_false;
	}

	if (auth_id == NULL) {
		error ("auth_id is NULL, unable to perform authentication");
		return axl_false;
	} /* end if */

	/* get a reference to the context */
	ctx = sasl_backend->ctx;

	msg ("Requesting to auth=%s, over conn-id=%d (serverName: %s)", auth_id, vortex_connection_get_id (conn), serverName ? serverName : "");

	/* lock the mutex */
	LOCK;

	/* check serverName authentication */
	if (serverName != NULL) {
		msg ("check SASL databases for serverName=%s", serverName);
		/* try to find the associated database */
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	} /* end if */

	/* if no database found, use the default one */
	if (db == NULL) {
		msg ("no especific serverName database found, using default: %p", sasl_backend->default_db);
		db = sasl_backend->default_db;
	}

	/* check database */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;

		error ("no sasl <auth-db> was found for the provided serverName or no default <auth-db> was found, unable to perform SASL authentication.");
		return axl_false;
	} /* end if */

	/* now we have the database, check the user and password */
	/* prepare key and password to be looked up */
	switch (db->format) {
	case SASL_STORAGE_FORMAT_MD5:
		/* redifine values */
		password = vortex_tls_get_digest (VORTEX_MD5, password);
		release  = axl_true;
		break;
	case SASL_STORAGE_FORMAT_SHA1:
		/* redifine values */
		password = vortex_tls_get_digest (VORTEX_SHA1, password);
		release  = axl_true;
		break;
	case SASL_STORAGE_FORMAT_PLAIN:
		/* plain do not require additional format */
		release  = axl_false;
		break;
	default:

		/* unlock the mutex */
		UNLOCK;

		/* error, unable to find the proper keying material
		 * encoding configuration */
		error ("unable to find the proper format for keying material (inside sasl.conf)");
		return axl_false;
	} /* end switch */

	/* now, according to the database backend, call to the proper
	 * function */
	switch (db->type) {
	case SASL_BACKEND_XML:
		/* get result */
		result = common_sasl_auth_db_xml (ctx, db, auth_id, authorization_id, password);
		break;
	case SASL_BACKEND_FORMAT_HANDLER:
		/* get result from format handler */
		result = common_sasl_auth_format_handler (ctx, conn, sasl_backend, db, auth_id, authorization_id, password, serverName, NULL);
		break;
	default:
		/* no support db format found */
		break;
	} /* end switch */

	/* unlock the mutex */
	UNLOCK;

	/* check if the account is disabled to apply
	 * <mod-sasl/login-options/accounts-disabled> configuration */
	if (result == -1) 
		common_sasl_apply_accounts_disabled (ctx, sasl_backend, conn);

	/* check if the login process failed */
	if (result == 0) 
		common_sasl_apply_max_allowed_tries (ctx, sasl_backend, conn);

	/* check to release memory allocated */
	if (release)
		axl_free ((char*) password);

	/* return auth operation */
	return result == 1;
}

/** 
 * @brief Allows to check if the provided sasl method is allowed. Only
 * method that is currently supported is "plain".
 * 
 * @param sasl_backend The sasl backend where to check for sasl
 * methods.
 *
 * @param sasl_method The sasl method to be checked to be
 * activated. Currently only "plain" is accepted.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the SASL method requested is supported.
 */
int  common_sasl_method_allowed   (SaslAuthBackend  * sasl_backend,
				   const char       * sasl_method,
				   VortexMutex      * mutex)
{
	axlNode * node;

	/* check receiving data */
	if (sasl_backend == NULL || sasl_method == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	node = axl_doc_get (sasl_backend->sasl_xml_conf, "/mod-sasl/method-allowed/method");
	while (node != NULL) {

		/* check for plain profile */
		if (HAS_ATTR_VALUE (node, "value", "plain")) {
			/* unlock the mutex */
			UNLOCK;

			/* accept plain profile */
			return axl_true;
		} /* end if */
		
		/* get the next node */
		node = axl_node_get_next (node);

	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return axl_false;
}

/** 
 * @brief Allows to check if the provided user already exists in the
 * database associated to the provided serverName.
 * 
 * @param sasl_backend The SASL backend where the search operation
 * will be performed.
 *
 * @param auth_id The user id to check.
 *
 * @param serverName The server name to configure the user database
 * where to lookup. If the value provided is null, the default
 * database will be used.
 * 
 * @param err An optional reference to an axlError where an textual
 * diagnostic error is returned if the function fails.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user already exists, otherwise axl_false is
 * returned.
 */
int  common_sasl_user_exists      (SaslAuthBackend   * sasl_backend,
				   const char        * auth_id,
				   const char        * serverName,
				   axlError         ** err,
				   VortexMutex       * mutex)
{
	axlNode    * node;
	const char * user_id;
	SaslAuthDb * db;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL ||
	    auth_id      == NULL) {
		axl_error_new (-1, "SASL backend or auth id are null, unable to operate", NULL, err);
		return axl_false;
	}

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;

		axl_error_new (-1, "Failed to find associated auth db, the user is not valid or the serverName configuration is not present.", NULL, err);
		return axl_false;
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		node     = axl_doc_get ((axlDoc*) db->db, "/sasl-auth-db/auth");

		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");

			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {
				/* unlock the mutex */
				UNLOCK;

				return axl_true;
			} /* end if */
			
			/* get next node */
			node     = axl_node_get_next (node);

		} /* end while */

		/* unlock the mutex */
		UNLOCK;

		axl_error_new (-1, "Failed to find the user, but found the associated backend.", NULL, err);
		return axl_false;
	} /* end if */

	axl_error_new (-1, "Failed to find the user.", NULL, err);

	/* unlock the mutex */
	UNLOCK;

	return axl_false;
}

/** 
 * @brief Allows to check if the serverName provided recognized
 * explicitly by the sasl backend. If the function returns axl_true that
 * means there is a auth-db configuration activated for the provided
 * serverName. 
 * 
 * @param sasl_backend The SASL backend where the search operation
 * will be performed.
 *
 * @param serverName The server name to configure the user database
 * where to lookup. If the value provided is null, the default
 * database will be used.
 * 
 * @param err An optional reference to an axlError where an textual
 * diagnostic error is returned if the function fails.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user already exists, otherwise axl_false is
 * returned.
 */
axl_bool       common_sasl_serverName_exists (SaslAuthBackend   * sasl_backend,
					      const char        * serverName,
					      axlError         ** err,
					      VortexMutex       * mutex)
{
	int  result;


	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL ||
	    serverName   == NULL) {
		axl_error_new (-1, "SASL backend or serverName are null, unable to operate", NULL, err);
		return axl_false;
	}

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	result = axl_hash_exists (sasl_backend->dbs, (axlPointer) serverName);

	/* unlock the mutex */
	UNLOCK;

	return result;
}

/** 
 * @brief Allows to check if there is a SASL default database backend
 * loaded on the provided SaslAuthBackend reference.
 *
 * @param sasl_backend The SASL backend where the operation will be
 * implemented.
 *
 * @param err An optional reference to an axlError where an textual
 * diagnostic error is returned if the function fails.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true the SASL backend has a default database or NULL if
 * it fails.
 */
axl_bool        common_sasl_has_default       (SaslAuthBackend   * sasl_backend,
					       axlError         ** err,
					       VortexMutex       * mutex)
{
	/* check if it has a reference and default defined */
	return (sasl_backend && sasl_backend->default_db);
}

/** 
 * @brief Common implementation to encode password stored.
 * 
 * @param format The format to be used.
 * @param password 
 * @param release 
 * 
 * @return 
 */
char * _common_sasl_encode_password (TurbulenceCtx      * ctx, 
				     SaslStorageFormat    format, 
				     const char         * password, 
				     int                * release)
{
	char * enc_password = NULL;

	/* according to the format encode */
	switch (format) {
	case SASL_STORAGE_FORMAT_MD5:
		/* redifine values */
		enc_password = vortex_tls_get_digest (VORTEX_MD5, password);
		*release     = axl_true;
		break;
	case SASL_STORAGE_FORMAT_SHA1:
		/* redifine values */
		enc_password = vortex_tls_get_digest (VORTEX_SHA1, password);
		*release     = axl_true;
		break;
	case SASL_STORAGE_FORMAT_PLAIN:
		/* plain do not require additional format */
		enc_password = (char*) password;
		*release     = axl_false;
		break;
	default:
		/* option not supported */
		/* error, unable to find the proper keying material
		 * encoding configuration */
		error ("unable to find the proper format for keying material (inside sasl.conf)");
		*release = axl_false;
		break;
	} /* end switch */

	return enc_password;
	
} /* end _common_sasl_encode_password */

/** 
 * @brief Allows to add the provided user under the database
 * associated to the provided serverName. 
 *
 * If the provided serverName is null, the user will be added in the
 * default user database.
 * 
 * @param sasl_backend The sasl database backend to be used to store
 * the user.
 *
 * @param auth_id The SASL user id to be used.
 *
 * @param password The password associated.
 *
 * @param serverName The server name used to identify the proper user
 * database.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user was added, otherwise axl_false is returned.
 */
int  common_sasl_user_add         (SaslAuthBackend  * sasl_backend, 
				   const char       * auth_id, 
				   const char       * password, 
				   const char       * serverName, 
				   VortexMutex      * mutex)
{
	axlNode    * node;
	axlNode    * newNode;
	char       * enc_password;
	int          release;
	SaslAuthDb * db;
	int          result = axl_false;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL ||
	    auth_id      == NULL ||
	    password     == NULL)
		return axl_false;

	/* before continue, check if the user already exists */
	if (common_sasl_user_exists (sasl_backend, auth_id, serverName, NULL, mutex))
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;
		return axl_false;
	}

	/* encode the password received according to the encoding
	 * configured */
	enc_password = _common_sasl_encode_password (sasl_backend->ctx, db->format, password, &release);
	if (enc_password == NULL) {
		/* unlock the mutex */
		UNLOCK;
		
		/* failed to encode */
		return axl_false;
	} /* end switch */

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* get the top root database node */
		node     = axl_doc_get ((axlDoc*) db->db, "/sasl-auth-db");
		if (node != NULL) {
			newNode  = axl_node_create ("auth");
	
			/* set user */
			axl_node_set_attribute (newNode, "user_id", auth_id);
			
			/* set password */
			axl_node_set_attribute (newNode, "password", enc_password);
			
			/* account enabled */
			axl_node_set_attribute (newNode, "disabled", "no");
			
			/* set the node */
			axl_node_set_child (node, newNode);
			
			/* dump the db */
			result = axl_doc_dump_pretty_to_file ((axlDoc *) (db->db), db->db_path, 3);
		} /* end if */

	} /* end if */

	if (release) 
		axl_free ((char*) enc_password);

	/* unlock the mutex */
	UNLOCK;

	/* return result */
	return result;
}

/** 
 * @brief Allows to change password associated to the provided auth
 * id, inside the context of serverName. 
 * 
 * @param sasl_backend The sasl backend where the operation will be
 * performed.
 *
 * @param auth_id The sasl user id that will change its password.
 *
 * @param new_password The new password to configure.
 *
 * @param serverName The serverName context the operation will be
 * performed.
 *
 * @param mutex Optional mutex to lock the backend implementation.
 * 
 * @return axl_true if the operation was performed, otherwise axl_false is
 * returned.
 */
int       common_sasl_user_password_change (SaslAuthBackend * sasl_backend,
					    const char      * auth_id,
					    const char      * new_password,
					    const char      * serverName,
					    VortexMutex     * mutex)
{
	/* reference to the turbulence context */
	TurbulenceCtx * ctx = NULL;
	axlNode       * node;
	const char    * user_id;
	SaslAuthDb    * db;
	char          * enc_password;
	int             release = axl_false;
	int             result;

	/* return if minimum parameters aren't found. */
	if (sasl_backend      == NULL ||
	    auth_id           == NULL ||
	    sasl_backend->ctx == NULL) {
		return axl_false;
	} /* end if */

	/* get a reference to the context */
	ctx = sasl_backend->ctx;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;
		
		return axl_false;
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		node     = axl_doc_get ((axlDoc*) db->db, "/sasl-auth-db/auth");

		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");

			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {

				/* user found change password */
				enc_password = _common_sasl_encode_password (ctx, db->format, new_password, &release);
				
				/* replace attribute */
				axl_node_remove_attribute (node, "password");
				axl_node_set_attribute (node, "password", enc_password);
				if (! HAS_ATTR_VALUE (node, "password", enc_password)) {
					/* unlock the mutex */
					UNLOCK;

					return axl_false;
				}

				/* release if signaled */
				if (release)
					axl_free (enc_password);

				/* dump the db */
				result = axl_doc_dump_pretty_to_file ((axlDoc*) db->db, db->db_path, 3);

				/* unlock the mutex */
				UNLOCK;

				return result;
			} /* end if */
			
			/* get next node */
			node     = axl_node_get_next (node);

		} /* end while */

		/* unlock the mutex */
		UNLOCK;
		
		return axl_false;
	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return axl_false;
}

/** 
 * @brief Allows to change the associated sasl user id, keeping all
 * settings associated.
 * 
 * @param sasl_backend The sasl backend where the operation will be
 * performed.
 *
 * @param auth_id The associated sasl auth id to be edited.
 *
 * @param new_auth_id The new sasl auth id to be placed.
 *
 * @param serverName The serverName context that identifies the database to use.
 *
 * @param mutex Optional mutex used to lock the backend while
 * operating.
 * 
 * @return axl_true if the edit operation was properly done, otherwise
 * axl_false if it fails. The operation will also fails if the user
 * doesn't exists.
 */
int       common_sasl_user_edit_auth_id       (SaslAuthBackend  * sasl_backend, 
					       const char       * auth_id, 
					       const char       * new_auth_id,
					       const char       * serverName, 
					       VortexMutex      * mutex)
{
	/* get a reference to the turbulence context */
	TurbulenceCtx * ctx = NULL;
	axlNode       * node;
	SaslAuthDb    * db;
	axlDoc        * auth_db;
	int             result = axl_true;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL || sasl_backend->ctx == NULL)
		return axl_false;

	/* get a reference to the context */
	ctx = sasl_backend->ctx;

	/* change if both user ids are equal */
	if (axl_cmp (auth_id, new_auth_id))
		return axl_true;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found, so the user doesn't
	 * exists, hence it is at least disabled */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;
		return axl_true; 
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {

			if (axl_cmp (ATTR_VALUE (node, "user_id"), auth_id)) {

				/* user found, change its value */
				axl_node_remove_attribute (node, "user_id");
				axl_node_set_attribute (node, "user_id", new_auth_id);

				/* remove the user from the remote administration interface */
				if (turbulence_db_list_exists (db->allowed_admins, auth_id)) {
					turbulence_db_list_remove (db->allowed_admins, auth_id);
					turbulence_db_list_add (db->allowed_admins, new_auth_id);
				} /* end if */

				/* unlock the mutex */
				UNLOCK;
				
				return axl_true;
			} /* end if */

			/* get next node */
			node     = axl_node_get_next (node);
		
		} /* end while */

	} /* end if */

	/* unlock the mutex */
	UNLOCK;
		
	/* return axl_true, the user is disabled mainly because it
	 * doesn't exists */
	return result;
}

/** 
 * @brief Allows to disable the selected used in the database
 * associated to the provided serverName. If the server name value is
 * NULL the user will be disabled in the default datbase.
 * 
 * @param sasl_backend The SASL backend where the operation will be
 * performed.
 * 
 * @param auth_id The user id to disable.
 *
 * @param serverName The serverName to use to select the proper user database.
 *
 * @param disable axl_true to disable the user, axl_false to enable.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user was disable, otherwise axl_false is returned.
 */
int  common_sasl_user_disable     (SaslAuthBackend  * sasl_backend, 
				   const char       * auth_id, 
				   const char       * serverName, 
				   int                disable,
				   VortexMutex      * mutex)
{
	axlNode    * node;
	SaslAuthDb * db;
	axlDoc     * auth_db;
	const char * user_id;
	int          result = axl_false;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL ||
	    auth_id      == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {

		/* unlock the mutex */
		UNLOCK;
		return axl_false;
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");
			
			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {
				
				/* user found, disable it */
				axl_node_remove_attribute (node, "disabled");
				
				/* install the new attribute */
				axl_node_set_attribute (node, "disabled", disable ? "yes" : "no");
				
				/* dump the db */
				result = axl_doc_dump_pretty_to_file (auth_db, db->db_path, 3);
				
				break;
			} /* end if */
		
			/* get next node */
			node     = axl_node_get_next (node);

		} /* end while */
	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return result;
}

/** 
 * @internal Function used to dealloc resource hold by the sasl user
 * structure.
 * 
 * @param user The user to dealloc.
 */
void common_sasl_user_free (SaslUser * user)
{
	if (user == NULL)
		return;
	axl_free (user->auth_id);
	axl_free (user);
	return;
}

/** 
 * @brief Allows to get the list of users stored in the provided SASL
 * backend using the serverName as selector for the proper database.
 * 
 * @param sasl_backend The sasl backend to oper.
 *
 * @param serverName The serverName to select database.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return A reference to the list of users created. Use \ref
 * axl_list_free to terminate the list.
 */
axlList * common_sasl_get_users      (SaslAuthBackend  * sasl_backend,
				      const char       * serverName,
				      VortexMutex      * mutex)
{
	axlNode    * node;
	SaslAuthDb * db;
	axlDoc     * auth_db;
	axlList    * list = NULL;
	SaslUser   * user;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;

		return NULL;
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		list     = axl_list_new (axl_list_always_return_1, (axlDestroyFunc) common_sasl_user_free);
		while (node != NULL) {

			/* create the user node */
			user    = axl_new (SaslUser, 1);
			
			/* get the user */
			user->auth_id  = axl_strdup (ATTR_VALUE (node, "user_id"));
			user->disabled = HAS_ATTR_VALUE (node, "disabled", "yes");
			
			/* store in the resulting list */
			axl_list_add (list, user); 
		
			/* get next node */
			node     = axl_node_get_next (node);
		
		} /* end while */

	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	/* returning the list */
	return list;
}

/** 
 * @brief Allows to check if the provided user is disabled (exists in
 * the datbase but no authorization can be done).
 * 
 * @param sasl_backend The sasl backend where the user will be check
 * to be disabled or not.
 *
 * @param auth_id The authorization id to be checked.
 *
 * @param serverName Server name configuration to select the proper
 * auth database.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user is disabled, otherwise axl_false is returned,
 * meaning the user can login.
 */
int       common_sasl_user_is_disabled (SaslAuthBackend  * sasl_backend,
					const char       * auth_id, 
					const char       * serverName,
					VortexMutex      * mutex)
{
	axlNode    * node;
	SaslAuthDb * db;
	axlDoc     * auth_db;
	int          result = axl_true;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found, so the user doesn't
	 * exists, hence it is at least disabled */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;
		return axl_true; 
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {

			if (axl_cmp (ATTR_VALUE (node, "user_id"), auth_id)) {


				/* return the current status for the is disabled */
				result = HAS_ATTR_VALUE (node, "disabled", "yes");
				break;
			} /* end if */

			/* get next node */
			node     = axl_node_get_next (node);
		
		} /* end while */

	} /* end if */

	/* unlock the mutex */
	UNLOCK;
		
	/* return axl_true, the user is disabled mainly because it
	 * doesn't exists */
	return result;
}

/** 
 * @brief Allows to enable remote administration on the provided
 * domain (serverName) for the provided user.
 * 
 * @param sasl_backend The sasl backend where the operation will take
 * place.
 *
 * @param auth_id The auth id that is going to be enabled/disabled to
 * perform remote administration.
 *
 * @param serverName The associated serverName where the operation
 * will take place.
 *
 * @param enable Activate or not the remote administration.
 *
 * @param mutex Optional mutex.
 * 
 * @return axl_true if the operation was completed, otherwise axl_false is
 * returned.
 */
int       common_sasl_enable_remote_admin  (SaslAuthBackend  * sasl_backend, 
					    const char       * auth_id, 
					    const char       * serverName,
					    int                enable,
					    VortexMutex      * mutex)
{
	TurbulenceCtx  * ctx = NULL;
	axlNode        * node;
	SaslAuthDb     * db;
	axlDoc         * auth_db;
	const char     * user_id;
	int              result = axl_false;

	/* return if minimum parameters aren't found. */
	if (sasl_backend      == NULL ||
	    auth_id           == NULL  ||
	    sasl_backend->ctx == NULL)
		return axl_false;

	/* get a reference to the context */
	ctx = sasl_backend->ctx;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {

		/* unlock the mutex */
		UNLOCK;
		return axl_false;
	}

	/* now activate the database remote administration, but first
	 * check if the user exists */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");
			
			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {

				/* according to the status, add or
				 * remove */
				if (enable) {
					/* user found, check if the already is
					 * located in the database */
					if (turbulence_db_list_exists (db->allowed_admins, auth_id)) {
						/* unlock the mutex */
						UNLOCK;
						
						return axl_true;
					}
					
					/* it doesn't exists, activate it */
					turbulence_db_list_add (db->allowed_admins, auth_id);
				} else {
					/* just remove */
					turbulence_db_list_remove (db->allowed_admins, auth_id);
				} /* end if */

				/* unlock the mutex */
				UNLOCK;
				
				return axl_true;
			} /* end if */
		
			/* get next node */
			node     = axl_node_get_next (node);

		} /* end while */
	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return result;	
}

/** 
 * @brief Allows to check if the provided user have the remote
 * administration activated.
 * 
 * @param sasl_backend The sasl backend where the operation will take place.
 *
 * @param auth_id The sasl auth id that is going to be activated to
 * support remote administration.
 *
 * @param serverName The serverName that defines the sasl database to be operated.
 *
 * @param mutex Optional mutex.
 * 
 * @return axl_true if the provided user (auth_id) has remote
 * administration activated on the provided serverName auth database.
 */
int       common_sasl_is_remote_admin_enabled (SaslAuthBackend  * sasl_backend,
					       const char       * auth_id, 
					       const char       * serverName,
					       VortexMutex      * mutex)
{
	TurbulenceCtx    * ctx = NULL;
	axlNode          * node;
	SaslAuthDb       * db;
	axlDoc           * auth_db;
	const char       * user_id;
	int                result = axl_false;

	/* return if minimum parameters aren't found. */
	if (sasl_backend      == NULL ||
	    auth_id           == NULL ||
	    sasl_backend->ctx == NULL)
		return axl_false;

	/* get a reference to the context */
	ctx = sasl_backend->ctx;
	
	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {

		/* unlock the mutex */
		UNLOCK;
		return axl_false;
	}
	
	/* now activate the database remote administration, but first
	 * check if the user exists */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");
			
			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {

				/* user found, check if the already is
				 * located in the database */
				result = turbulence_db_list_exists (db->allowed_admins, auth_id);

				/* unlock the mutex */
				UNLOCK;
				
				return result;
			} /* end if */
		
			/* get next node */
			node     = axl_node_get_next (node);

		} /* end while */
	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return axl_false;
}

/** 
 * @brief Allows to remove the provided user under the database
 * selected the given serverName.
 * 
 * @param sasl_backend The sasl backend where the operation will take
 * place.
 *
 * @param auth_id The user id to remove.
 *
 * @param serverName The server name value to use to select the proper
 * user database.
 *
 * @param mutex An optional mutex used by the library to lock the
 * database while operating.
 * 
 * @return axl_true if the user is removed, otherwise axl_false is returned.
 */
int       common_sasl_user_remove    (SaslAuthBackend  * sasl_backend,
				      const char       * auth_id, 
				      const char       * serverName, 
				      VortexMutex      * mutex)
{
	axlNode    * node;
	SaslAuthDb * db;
	axlDoc     * auth_db;
	const char * user_id;
	int          result = axl_false;

	/* return if minimum parameters aren't found. */
	if (sasl_backend == NULL ||
	    auth_id      == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* get the appropiate database */
	if (serverName == NULL)
		db = sasl_backend->default_db;
	else {
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}
	
	/* check if the database was found */
	if (db == NULL) {
		/* unlock the mutex */
		UNLOCK;

		return axl_false;
	}

	/* according to the database backend, do */
	if (db->type == SASL_BACKEND_XML) {
		
		/* search the user */
		auth_db  = (axlDoc *) db->db;
		node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
		while (node != NULL) {
			
			/* get the user */
			user_id = ATTR_VALUE (node, "user_id");
			
			/* check the user id */
			if (axl_cmp (auth_id, user_id)) {

				/* remove the node */
				axl_node_remove (node, axl_true);
				
				/* dump the db */
				result = axl_doc_dump_pretty_to_file (auth_db, db->db_path, 3);
				
				break;
			} /* end if */
		
			/* get next node */
			node     = axl_node_get_next (node);
		
		} /* end while */

	} /* end if */

	/* unlock the mutex */
	UNLOCK;

	return result;
}

/** 
 * @internal (Re)Loads the xml users database into memory.
 * 
 * @return axl_true if the db was properly loaded.
 */
axl_bool  common_sasl_load_users_db (TurbulenceCtx  * ctx, 
				     SaslAuthDb     * db, 
				     VortexMutex    * mutex)
{
	axlError * error;
	
	/* check received parameter */
	if (db == NULL)
		return axl_false;

	/* lock the mutex */
	LOCK;

	/* check file modification */
	if (turbulence_last_modification (db->db_path) == db->db_time) {
		/* unlock the mutex */
		UNLOCK;
		return axl_true;
	} /* end if */

	/* free the document if defined */
	if (db->db) {
		wrn ("Reloading SASL xml database due to file modification update");
		axl_doc_free (db->db);
	}

	/* find the file to load */
	db->db       = axl_doc_parse_from_file (db->db_path, &error);
	
	/* check db opened */
	if (db->db == NULL) {
		/* unlock the mutex */
		UNLOCK;

		error ("failed to init the SASL profile, unable to auth db, error: %s",
		       axl_error_get (error));
		axl_error_free (error);
		return axl_false;
	} /* end if */

	/* get current db time */
	db->db_time = turbulence_last_modification (db->db_path);

	/* unlock the mutex */
	UNLOCK;

	return axl_true;
}


/** 
 * @brief Allows to check if there is a remote administration
 * interface activated for some database. 
 * 
 * @param sasl_backend 
 * @param mutex 
 * 
 * @return The function returns axl_true if there are at least one remote
 * administration activated.
 */
int       common_sasl_activate_remote_admin (SaslAuthBackend * sasl_backend,
					     VortexMutex     * mutex)
{
	axlHashCursor * cursor;
	SaslAuthDb    * db;

	/* lock during operation */
	LOCK;

	/* check remote administration on default database */
	if (sasl_backend->default_db != NULL && 
	    sasl_backend->default_db->remote_admin) {
		/* unlock the mutex */
		UNLOCK;

		return axl_true;
	}

	/* check for the rest of auth dbs */
	cursor = axl_hash_cursor_new (sasl_backend->dbs);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get the database */
		db = axl_hash_cursor_get_value (cursor);

		if (db->remote_admin) {
			/* found remote admin activated, dealloc
			 * cursor */
			axl_hash_cursor_free (cursor);
		
			/* unlock the mutex */
			UNLOCK;

			return axl_true;
		} /* end if */

		/* next item to explore */
		axl_hash_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_hash_cursor_free (cursor);

	/* unlock the mutex */
	UNLOCK;

	return axl_false;
}

/** 
 * @internal SASL remote administration API. This function is executed
 * to validate the service invocation against a particular user
 * database, based on the value provided by serverName and the current
 * authorization Id. It also takes into account the current SASL
 * associated the list of remote admins allowed.
 * 
 * @param conn The connection where the verification is taking place.
 *
 * @param channel_num The channel number to be created.
 *
 * @param serverName The server name that is requested. This is the
 * critical value that is pointing to the database to admin.
 *
 * @param resource_path The resource_path requested.
 *
 * @param user_data On this parameter is received the sasl_backend
 * being used.
 * 
 * @return axl_true to accept the request, otherwise axl_false is returned.
 */
int       common_sasl_validate_resource (VortexConnection * conn,
					 int                channel_num,
					 const char       * serverName,
					 const char       * resource_path,
					 axlPointer         user_data)
{
	const char       * auth_id;
	SaslAuthBackend  * sasl_backend = user_data;
	TurbulenceCtx    * ctx = NULL;
	SaslAuthDb       * db;

	/* do not validate if a null reference is received */
	if (sasl_backend == NULL)
		return axl_false;

	/* get a reference to the context */
	ctx = sasl_backend->ctx;

	/* check resource associated to the sasl remote
	 * administration */
	if (! axl_cmp (resource_path, "sasl-radmin")) {
		/* undefined resource, we are not handling such
		 * resource */
		return axl_false;
	}
	
	/* check the user id */
	auth_id = AUTH_ID_FROM_CONN (conn);
	if (auth_id == NULL) {
		error ("Requested validation for remote SASL administration but no SASL credential was found");
		return axl_false;
	} /* end if */

	/* now check the database */
	if (serverName == NULL) {
		/* requested validation for the default database, get
		 * a reference to check its remote admin support */
		db = sasl_backend->default_db;
	} else {
		/* requested validation for a particular database, get
		 * a reference */
		db = axl_hash_get (sasl_backend->dbs, (axlPointer) serverName);
	}

	/* check to report errors */
	if (db == NULL) {
		error ("Unable to find associated database for the serverName='%s'",
		       serverName ? serverName : "none");
		return axl_false;
	} /* end if */

	if (! db->remote_admin) {
		error ("Remote SASL admin is disabled on database associated to serverName='%s'",
		       serverName ? serverName : "none");
		return axl_false;
	}

	if (! turbulence_db_list_exists (db->allowed_admins, auth_id)) {
		error ("Requested to manage a SASL db associated to serverName='%s' with auth_id='%s' not allowed",
		       serverName ? serverName : "none",
		       auth_id ? auth_id : "none");
		return axl_false;
	}

	/* now check remote admin support (the following code relay on
	 * the fact that the associated database was found with a
	 * remote admin list properly configured and the remote admin
	 * flag activated). */
	if (db != NULL && db->remote_admin && turbulence_db_list_exists (db->allowed_admins, auth_id)) {
		msg ("accepted xml-rpc SASL remote administration for %s domain",
		     serverName ? serverName : "default");
		return axl_true;
	}

	/* return axl_false */
	return axl_false;
}

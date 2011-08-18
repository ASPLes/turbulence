/* mod_sasl_mysql implementation */
#include <turbulence.h>

/* mysql flags */
#include <mysql.h>

/* include support for common-sasl */
#include <common-sasl.h>

/* include dtd definition */
#include <mysql.sasl.dtd.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* global dtd used to validate document defining settings to open
   mysql database */
axlDtd        * mysql_sasl_dtd = NULL;

/** 
 * @internal Function that creates a connection to the MySQL database
 * configured on the xml node.
 */ 
MYSQL * mod_sasl_mysql_get_connection (TurbulenceCtx  * ctx,
				       axlNode        * auth_db_node_conf, 
				       axlError      ** err)
{
	MYSQL   * conn;
	int       port = 0;
	int       reconnect = 1;
	axlDoc  * doc;
	axlNode * node;

	if (ctx == NULL || auth_db_node_conf == NULL) {
		axl_error_report (err, -1, "Received null ctx, auth db node or sql query, failed to run SQL command");
		return NULL;
	} /* end if */

	/* check if the connection is already defined */
	conn = axl_node_annotate_get (auth_db_node_conf, "mysql-conn", axl_false);
	if (conn) {
		/* reuse connection */
		return conn;
	} /* end if */

	/* get document containing MySQL settings */
	doc  = axl_node_annotate_get (auth_db_node_conf, "mysql-conf", axl_false);
	if (doc == NULL) {
		axl_error_report (err, -1, "Found no xml document defining MySQL settings to connect to the database");
		return NULL;
	} /* end if */

	/* get the node that contains the configuration */
	node = axl_doc_get (doc, "/sasl-auth-db/connection-settings");

	/* create a mysql connection */
	conn = mysql_init (NULL);

	/* get port */
	if (HAS_ATTR (node, "port") && strlen (ATTR_VALUE (node, "port")) > 0) {
		/* get port configured by the user */
		port = atoi (ATTR_VALUE (node, "port"));
	}
	
	/* create a connection */
	if (mysql_real_connect (conn, 
				/* get host */
				ATTR_VALUE (node, "host"), 
				/* get user */
				ATTR_VALUE (node, "user"), 
				/* get password */
				ATTR_VALUE (node, "password"), 
				/* get database */
				ATTR_VALUE (node, "database"), 
				port, NULL, 0) == NULL) {
		axl_error_report (err, mysql_errno (conn), "Mysql connect error: %s, failed to run SQL command", mysql_error (conn));
		return NULL;
	} /* end if */

	/* flag here to reconnect in case of lost connection */
	mysql_options (conn, MYSQL_OPT_RECONNECT, (const char *) &reconnect);

	/* record connection */
	axl_node_annotate_data_full (auth_db_node_conf, "mysql-conn", NULL, conn, (axlDestroyFunc) mysql_close);

	return conn;
}

/** 
 * @internal Function that makes a SQL connection to the configured
 * database and return a MYSQL_RES object that contains the result or
 * axl_true in the case non_query is axl_true.
 *
 * With the result created, the caller must do:
 *
 * \code
 * MYSQL_ROW row;
 *
 * // get a cell data
 * row = mysql_fetch_row (result);
 * row[i] -> each field.
 *
 * // to release 
 * mysql_free_result (result);
 * \endcode
 */
MYSQL_RES * mod_sasl_mysql_do_query (TurbulenceCtx  * ctx, 
				     axlNode        * auth_db_node_conf,
				     const char     * sql_query,
				     axl_bool         non_query,
				     axlError      ** err)
{  
	MYSQL     * conn;

	/* check sql connection */
	if (sql_query == NULL) {
		axl_error_report (err, -1, "Unable to run SQL query, received NULL content, failed to run SQL command");
		return NULL;
	} /* end if */

	/* get connection */
	conn = mod_sasl_mysql_get_connection (ctx, auth_db_node_conf, err);
	if (conn == NULL) {
		axl_error_report (err, -1, "Failed to get connection to MySQL database. Unable to execute query: %s", sql_query);
		return NULL;
	}

	/* now run query */
	if (mysql_query (conn, sql_query)) {
		axl_error_report (err, mysql_errno (conn), "Failed to run SQL query, error was %u: %s\n", mysql_errno (conn), mysql_error (conn));
		return NULL;
	} /* end if */
	
	/* check if this is a non query and return proper status now */
	if (non_query)
		return INT_TO_PTR (axl_true);

	/* return result created */
	return mysql_store_result (conn);
}

axl_bool mod_sasl_mysql_check_ip_filter_query (TurbulenceCtx     * ctx,
					       const char        * query, 
					       VortexConnection  * conn,
					       axlNode           * auth_db_node_conf) {

	MYSQL_RES      * result;
	MYSQL_ROW        row;
	axlError       * err = NULL;
	TurbulenceExpr * expr;

	/* run query */
	result = mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_false, &err);

	/* check result */
	if (result == NULL) {
		error ("Unable to run ip filter SQL, query string failed with %s", axl_error_get (err));
		axl_error_free (err);
		return axl_false; 
	} /* end if */

	/* return content from the first [0][0] array position */
	row     = mysql_fetch_row (result);
	if (row == NULL) {
		mysql_free_result (result);
		return axl_false;
	} /* end if */

	/* check for empty filter string */
	if (row[0] == NULL || strlen (row[0]) == 0) {
		mysql_free_result (result);
		return axl_true;
	}
	msg ("Checking to apply ip filter with expression: %s (ip: %s:%s)", row[0], 
	     vortex_connection_get_host (conn), vortex_connection_get_host_ip (conn));

	/* build expression */
	expr = turbulence_expr_compile (ctx, row[0], NULL);
	if (expr == NULL) {
		error ("Failed to compile expression: %s. Unable to apply ip filter, denying connection.", row[0]);
		mysql_free_result (result);
		return axl_false; /* do not filter */
	}
	mysql_free_result (result);

	/* now match by hostname  */
	if (turbulence_expr_match (expr, vortex_connection_get_host (conn))) {
		turbulence_expr_free (expr);
		return axl_true; /* do not filter */
	}

	/* ..and by hostip */
	if (turbulence_expr_match (expr, vortex_connection_get_host_ip (conn))) {
		turbulence_expr_free (expr);
		return axl_true; /* do not filter */
	}

	/* free expression */
	turbulence_expr_free (expr);
	return axl_false; /* do filter */
}

axl_bool mod_sasl_mysql_do_auth (TurbulenceCtx    * ctx, 
				 VortexConnection * conn,
				 axlNode          * auth_db_node_conf,
				 const char       * auth_id,
				 const char       * authorization_id,
				 const char       * password,
				 const char       * serverName,
				 const char       * sasl_method,
				 axlError        ** err)
{
	char        * query; 
	axlDoc      * doc;
	axlNode     * node;
	MYSQL_RES   * result;
	MYSQL_ROW     row;
	axl_bool      _result;

	/* get the auth query */
	doc  = axl_node_annotate_get (auth_db_node_conf, "mysql-conf", axl_false);
	if (doc == NULL) {
		axl_error_report (err, -1, "Found no xml document defining MySQL settings to connect to the database");
		return axl_false;
	} /* end if */

	/* check for ip filter reference */
	node  = axl_doc_get (doc, "/sasl-auth-db/ip-filter");
	if (node && HAS_ATTR (node, "query")) {
		/* ip filter defined, get query */
		query = axl_strdup (ATTR_VALUE (node, "query"));

		/* replace query with recognized tokens */
		axl_replace (query, "%u", auth_id);
		axl_replace (query, "%n", serverName);
		axl_replace (query, "%i", authorization_id);
		axl_replace (query, "%m", sasl_method);
		
		if (! mod_sasl_mysql_check_ip_filter_query (ctx, query, conn, auth_db_node_conf)) {
			error ("IP filtered by defined expression associated to user: %s denied connection from %s", 
			       auth_id, vortex_connection_get_host_ip (conn));
			axl_free (query);
			return 0;
		}
		msg ("IP not filtered by defined expression associated to user: %s allowed connection from %s", 
		       auth_id, vortex_connection_get_host_ip (conn));
		
		/* ip not filtered, now let the auth continue */
		axl_free (query);
	} /* end if */

	/* get the node that contains the configuration */
	node  = axl_doc_get (doc, "/sasl-auth-db/get-password");
	query = axl_strdup (ATTR_VALUE (node, "query"));
	
	/* replace query with recognized tokens */
	axl_replace (query, "%u", auth_id);
	axl_replace (query, "%n", serverName);
	axl_replace (query, "%i", authorization_id);
	axl_replace (query, "%m", sasl_method);
	axl_replace (query, "%p", vortex_connection_get_host (conn));

	msg ("Trying to auth %s with query string %s", auth_id, query);

	/* run query */
	result = mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_false, err);
	axl_free (query);
	/* check result */
	if (result == NULL) {
		error ("Unable to authenticate user, query string failed with %s", axl_error_get (*err));
		axl_error_free (*err);
		return 0; 
	} /* end if */

	/* return content from the first [0][0] array position */
	row     = mysql_fetch_row (result);
	if (row == NULL) {
		mysql_free_result (result);
		return 0;
	} /* end if */
	/* check result */
	_result = axl_cmp (row[0], password);
	mysql_free_result (result);

	/* now check for auth-log declaration to report it */
	node = axl_doc_get (doc, "/sasl-auth-db/auth-log");
	if (node) {
		/* log auth defined */
		query = axl_strdup (ATTR_VALUE (node, "query"));
	
		/* replace query with recognized tokens */
		axl_replace (query, "%t", _result ? "ok" : "failed");
		axl_replace (query, "%u", auth_id);
		axl_replace (query, "%n", serverName);
		axl_replace (query, "%i", authorization_id);
		axl_replace (query, "%m", sasl_method);
		axl_replace (query, "%p", vortex_connection_get_host (conn));
		
		msg ("Trying to auth-log %s:%s with query string %s", auth_id, _result ? "ok" : "failed", query);
		/* exec query */
		if (! mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_true, err)) {
			error ("Unable to auth-log, failed query configured, error was: %d:%s",
			       axl_error_get_code (*err), axl_error_get (*err));
			axl_error_free (*err);
		}
		axl_free (query);
		
	} /* end if */
	
	return _result ? 1 : 0;
}

axl_bool mod_sasl_mysql_load_auth_db (TurbulenceCtx     * ctx,
				      SaslAuthBackend   * sasl_backend,
				      axlNode           * auth_db_node_conf,
				      axlError         ** err)
{
	MYSQL      * conn;
	const char * location;
	char       * basedir = NULL;
	axlDoc     * doc;
	axlError   * local_err = NULL;

	/* check if location is defined */
	if (! HAS_ATTR (auth_db_node_conf, "location")) {
		axl_error_report (err, -1, "Unable to open auth mysql database, 'location' attribute is not defined");
		return axl_false;
	} /* end if */

	/* find the node that holds the connection configuration */
	location = ATTR_VALUE (auth_db_node_conf, "location");

	/* check if the location is relative or not */
	if (! turbulence_file_is_fullpath (location)) {
		/* get base dir of the sasl.conf that represents this
		   backend */
		basedir  = turbulence_base_dir (common_sasl_get_file_path (sasl_backend));

		/* now build a new path */
		location = axl_strdup_printf ("%s%s%s", basedir, VORTEX_FILE_SEPARATOR, location);
		msg ("Found relative file to auth mysql db settings, resolved to: %s", location);
	} /* end if */

	/* now load the file */
	doc      = axl_doc_parse_from_file (location, &local_err);
	/* check for error and report */
	if (doc == NULL) {
		axl_error_report (err, -1, "Failed to open auth mysql db at %s error was %s", location, axl_error_get (local_err));
		axl_error_free (local_err);
	} /* end if */

	/* dealloc some variables */
	if (basedir) {
		axl_free (basedir);
		axl_free ((char *) location);
	} /* end if */

	/* return if error */
	if (doc == NULL) {
		return axl_false;
	} /* end if */

	/* do DTD validation */
        if (! axl_dtd_validate (doc, mysql_sasl_dtd, &local_err)) {
		axl_error_report (err, -1, "Failed to open auth mysql db at %s, found DTD error %s", location, axl_error_get (local_err));
		axl_error_free (local_err);
		axl_doc_free (doc);
		return axl_false;
	} /* end if */

	/* link the document to this node so we can reuse it later */
	axl_node_annotate_data_full (auth_db_node_conf, "mysql-conf", NULL, doc, (axlDestroyFunc) axl_doc_free);

	/* request to load msyql database */
	conn = mod_sasl_mysql_get_connection (ctx, auth_db_node_conf, err);
	if (conn == NULL) 
		return axl_false;

	msg ("load database ok");

	/* connection ok, this means we have loaded the database */
	return axl_true;
}

/** 
 * @internal Main entry point to resolve requests to mysql database
 * according to the operation requested.
 */
axlPointer mod_sasl_mysql_format_handler (TurbulenceCtx    * ctx,
					  VortexConnection * conn,
					  SaslAuthBackend  * sasl_backend,
					  axlNode          * auth_db_node_conf,
					  ModSaslOpType      op_type,
					  const char       * auth_id,
					  const char       * authorization_id,
					  const char       * password,
					  const char       * serverName,
					  const char       * sasl_method,
					  axlError        ** err,
					  VortexMutex      * mutex)
{
	switch (op_type) {
	case MOD_SASL_OP_TYPE_AUTH:
		/* request to auth user */
		return INT_TO_PTR (mod_sasl_mysql_do_auth (ctx, conn, auth_db_node_conf, 
							   auth_id, authorization_id, password, serverName, sasl_method, err));
	case MOD_SASL_OP_TYPE_LOAD_AUTH_DB:
		/* request to load database (check we can connect with current settings) */
		return INT_TO_PTR (mod_sasl_mysql_load_auth_db (ctx, sasl_backend, auth_db_node_conf, err));
	}
	return NULL;
}

/* mod_sasl_mysql init handler */
static int  mod_sasl_mysql_init (TurbulenceCtx * _ctx) {
	axlError * err = NULL;

	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* load DTD later used */
	if (mysql_sasl_dtd == NULL)
		mysql_sasl_dtd = axl_dtd_parse (MYSQL_SASL_DTD, -1, &err);
        if (mysql_sasl_dtd == NULL) {
		error ("failed to load mysql.sasl.dtd to check sasl configuration, error: %s",
		       axl_error_get (err));
		axl_error_free (err);
                return axl_false;
	} /* end if */

	/* install here all support to handle "mysql" databases */
	if (! common_sasl_register_format (_ctx, "mysql", mod_sasl_mysql_format_handler)) {
		axl_dtd_free (mysql_sasl_dtd);
		mysql_sasl_dtd = NULL;
		error ("Failed to register mod-sasl mysql database handler, register format function failed");
		return axl_false;
	}

	msg ("Registered mod-sasl mysql database handler OK");
	return axl_true;
} /* end mod_sasl_mysql_init */

/* mod_sasl_mysql close handler */
static void mod_sasl_mysql_close (TurbulenceCtx * _ctx) {
	msg ("Finishing mod-sasl MySQL extension..");
	/* frees up other memory used by the libmysqlclient. */
	mysql_library_end ();

	/* finish dtd used */
	axl_dtd_free (mysql_sasl_dtd);
	mysql_sasl_dtd = NULL;
	return;
} /* end mod_sasl_mysql_close */

/* mod_sasl_mysql reconf handler */
static void mod_sasl_mysql_reconf (TurbulenceCtx * _ctx) {
	/* Place here all your optional reconf code if the HUP signal is received */
	return;
} /* end mod_sasl_mysql_reconf */

/* mod_sasl_mysql unload handler */
static void mod_sasl_mysql_unload (TurbulenceCtx * _ctx) {
	/* Place here the code required to dealloc resources used by your module because turbulence signaled the child process must not have access */
	return;
} /* end mod_sasl_mysql_unload */

/* mod_sasl_mysql ppath-selected handler */
static axl_bool mod_sasl_mysql_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {
	/* Place here the code to implement all provisioning that was deferred because non enough data was available at init method (connection and profile path selected) */
	return axl_true;
} /* end mod_sasl_mysql_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_sasl_mysql",
	"MySQL authentication backend for MOD-SASL",
	mod_sasl_mysql_init,
	mod_sasl_mysql_close,
	mod_sasl_mysql_reconf,
	mod_sasl_mysql_unload,
	mod_sasl_mysql_ppath_selected
};

END_C_DECLS


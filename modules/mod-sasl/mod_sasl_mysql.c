/* mod_sasl_mysql implementation */
#include <turbulence.h>

/* mysql flags */
#include <mysql.h>

/* include support for common-sasl */
#include <common-sasl.h>

/* SASL properties: used to publish the resolved identity so the whole
 * stack (including language bindings) sees the session running as it */
#include <vortex_sasl.h>

/* include dtd definition */
#include <mysql.sasl.dtd.h>

/* configuration layer (pure, testable: see mod_sasl_mysql_conf.h) */
#include <mod_sasl_mysql_conf.h>

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

/* Return a newly allocated, SQL-escaped copy of 'value' using the active
 * MySQL connection (so the connection charset is honoured). Caller must
 * axl_free() the result. Returns NULL if value is NULL or on connection
 * error. */
char * mod_sasl_mysql_escape (TurbulenceCtx * ctx,
			      axlNode       * auth_db_node_conf,
			      const char    * value)
{
	MYSQL    * conn;
	char     * escaped;
	int        len;
	axlError * err = NULL;

	if (value == NULL)
		return NULL;

	conn = mod_sasl_mysql_get_connection (ctx, auth_db_node_conf, &err);
	if (conn == NULL) {
		error ("Unable to get MySQL connection to escape SQL value: %s",
		       err ? axl_error_get (err) : "<no error>");
		if (err)
			axl_error_free (err);
		return NULL;
	} /* end if */

	len     = strlen (value);
	escaped = axl_new (char, (len * 2) + 1);   /* worst case per MySQL API */
	if (escaped == NULL)
		return NULL;

	mysql_real_escape_string (conn, escaped, value, len);
	return escaped;
}

/* Replace 'token' in 'query' with the SQL-escaped form of 'value'. The
 * values interpolated into the SQL query (%u, %n, %i, %m, %p) MUST go
 * through this macro to be safe against SQL injection. Requires 'ctx'
 * and 'auth_db_node_conf' to be in scope. */
#define REPLACE_ESCAPED(query, token, value) do {                              \
		char * __esc = mod_sasl_mysql_escape (ctx, auth_db_node_conf,  \
						      (value));                \
		axl_replace ((query), (token), __esc ? __esc : "");            \
		if (__esc)                                                     \
			axl_free (__esc);                                      \
	} while (0)


/**
 * @internal Escape handler handed to the configuration layer so every
 * statement is built with the escaping of the live MySQL connection.
 */
char * mod_sasl_mysql_escape_handler (axlPointer data, const char * value)
{
	axlNode * auth_db_node_conf = (axlNode *) data;
	return mod_sasl_mysql_escape (ctx, auth_db_node_conf, value);
}

/**
 * @internal Convenience wrapper: builds a statement for this backend,
 * escaping through the live MySQL connection of [auth_db_node_conf].
 */
char * mod_sasl_mysql_build (axlNode * auth_db_node_conf, const char * query_template, ModSaslMysqlSubst * subst)
{
	return mod_sasl_mysql_build_query (query_template, subst, mod_sasl_mysql_escape_handler, auth_db_node_conf);
}

/**
 *
 */
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
		return axl_true; /* do not filter (user unknown, so let login fail) */
	} /* end if */

	/* check for empty filter string */
	if (row[0] == NULL || strlen (row[0]) == 0) {
		mysql_free_result (result);
		return axl_true; /* do not filter */
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

axl_bool __mod_sasl_mysql_prepare_query_and_auth (TurbulenceCtx    * ctx, 
						  const char       * _query,
						  VortexConnection * conn,
						  axlNode          * auth_db_node_conf,
						  const char       * auth_id,
						  const char       * authorization_id,
						  const char       * formated_password,
						  const char       * password,
						  const char       * serverName,
						  const char       * sasl_method,
						  axl_bool           just_run_query,
						  axl_bool           skip_login_error_reporting,
						  axlError        ** err)
{

	MYSQL_RES         * result;
	MYSQL_ROW           row;
	axl_bool            _result;
	char              * query;
	ModSaslMysqlSubst   subst;

	/* build the statement through the shared substitution helper so
	 * every declaration of auth-db.mysql.xml behaves identically */
	subst.auth_id          = auth_id;
	subst.serverName       = serverName;
	subst.authorization_id = authorization_id;
	subst.sasl_method      = sasl_method;
	subst.peer             = vortex_connection_get_host (conn);
	subst.status           = NULL;
	subst.effective_id     = NULL;

	query = mod_sasl_mysql_build (auth_db_node_conf, _query, &subst);
	if (query == NULL)
		return axl_false; /* allocation failure */

	if (! just_run_query) {
		msg ("Trying to auth [%s] with query string [%s], conn-id=%d from %s:%s ", auth_id, query, 
		     vortex_connection_get_id (conn), vortex_connection_get_host (conn), vortex_connection_get_port (conn));
	} /* end if */

	/* run query */
	result = mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_false, err);
	axl_free (query);

	/* check if we have to only run this query */
	if (just_run_query) {
		mysql_free_result (result);
		return axl_true;
	} /* end if */

	/* check result */
	if (result == NULL) {
		error ("Unable to authenticate user, query string failed with %s", axl_error_get (*err));
		axl_error_free (*err);
		return axl_false;
	} /* end if */

	/* return content from the first [0][0] array position */
	row     = mysql_fetch_row (result);
	if (row == NULL) {
		if (! skip_login_error_reporting) { 
			/* log login failure */
			error ("login failure: %s, failed from: %s", auth_id, vortex_connection_get_host_ip (conn));
		} /* end if */

		mysql_free_result (result);
		return  axl_false;
	} /* end if */
	/* check result */
	_result = axl_cmp (row[0], formated_password);
	if (! _result) {
		/* if it fails, check password format */
		/* support here passwords schemes using  */
		/* http://wiki.dovecot.org/Authentication/PasswordSchemes */
		_result = common_sasl_check_crypt_password (password, row[0]);
	} /* end if */
	mysql_free_result (result);

	return _result;
}
       

/* ---------------------------------------------------------------------
 * GENERIC EXTENSION POINTS
 *
 * Beyond the fixed set of statements this backend has always had
 * (<get-password>, <ip-filter>, <auth-log>...), auth-db.mysql.xml
 * accepts three families of declarations that may be repeated and are
 * evaluated in document order. They exist so a deployment can add
 * checks, identity mappings and notifications without this module
 * having to learn about each particular feature:
 *
 *   <auth-filter>   an additional condition that may deny the auth.
 *                   Generalises <ip-filter>.
 *
 *   <auth-resolve>  maps the authenticated credential onto the identity
 *                   the session must actually run as. This is what
 *                   allows API tokens (an opaque token id resolving to
 *                   the user it belongs to), and equally aliases,
 *                   tenant mapping or proxy identities.
 *
 *   <auth-notify>   fire and forget statement executed once the auth
 *                   result is known. Generalises <auth-log>.
 *
 * Every one of them shares the substitution semantics of
 * mod_sasl_mysql_build_query, so %u %n %i %m %p %t and %e mean the same
 * thing everywhere.
 * ------------------------------------------------------------------ */

/* connection data key holding the identity the session must run as,
 * when an <auth-resolve> declaration mapped it onto a different one.
 * Absent when the credential authenticated as itself. */
#define MOD_SASL_EFFECTIVE_ID_KEY "sasl:effective-auth-id"

/* connection data key holding the credential originally presented,
 * kept for auditing when the identity was resolved onto another one */
#define MOD_SASL_ORIGINAL_ID_KEY  "sasl:original-auth-id"

/**
 * @internal Runs the query and reports the first cell of the first row.
 *
 * @return 1 when a row with a non empty value was found (and [value]
 * holds a newly allocated copy), 0 when there was no row or it was
 * empty, and -1 when the statement failed.
 */
int mod_sasl_mysql_query_first_value (TurbulenceCtx * ctx,
				      axlNode       * auth_db_node_conf,
				      const char    * query,
				      char         ** value)
{
	MYSQL_RES * result;
	MYSQL_ROW   row;
	axlError  * err = NULL;

	if (value)
		(*value) = NULL;

	result = mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_false, &err);
	if (result == NULL) {
		error ("Failed to run statement [%s], error was: %s", query,
		       err ? axl_error_get (err) : "<no error>");
		if (err)
			axl_error_free (err);
		return -1;
	} /* end if */

	row = mysql_fetch_row (result);
	if (row == NULL || row[0] == NULL || strlen (row[0]) == 0) {
		mysql_free_result (result);
		return 0;
	} /* end if */

	if (value)
		(*value) = axl_strdup (row[0]);
	mysql_free_result (result);
	return 1;
}

/**
 * @internal Evaluates every <auth-filter> declared for the provided
 * stage ("pre-auth" by default, "post-auth" to run it once the
 * credential was accepted and the identity resolved).
 *
 * The way the returned row is interpreted is selected with the "match"
 * attribute:
 *
 *   expression (default) the value is compiled as a turbulence
 *                        expression and matched against the peer host
 *                        and address, exactly like <ip-filter>. No
 *                        value means no restriction.
 *   required             a non empty value must be returned, otherwise
 *                        the auth is denied. Useful for "the account
 *                        must still satisfy X" checks.
 *   forbidden            any non empty value denies the auth. Useful
 *                        for blacklists.
 *
 * A statement that fails always denies: an extension that cannot be
 * evaluated must never silently grant access.
 *
 * @return axl_true to let the auth continue, axl_false to deny it.
 */
axl_bool mod_sasl_mysql_run_filters (TurbulenceCtx     * ctx,
				     VortexConnection  * conn,
				     axlNode           * auth_db_node_conf,
				     axlDoc            * doc,
				     const char        * stage,
				     ModSaslMysqlSubst * subst)
{
	axlNode    * node;
	const char * match;
	const char * name;
	char       * template;
	char       * query;
	char       * value;
	int          status;
	axl_bool     allowed;

	node = axl_doc_get (doc, "/sasl-auth-db/auth-filter");
	while (node != NULL) {

		/* only the declarations of the requested stage */
		if (! mod_sasl_mysql_filter_applies (node, stage)) {
			node = axl_node_get_next_called (node, "auth-filter");
			continue;
		} /* end if */

		name  = mod_sasl_mysql_node_name (node);
		match = mod_sasl_mysql_filter_match (node);

		if (! mod_sasl_mysql_match_is_known (match)) {
			error ("login failure: %s, <auth-filter> [%s] declares an unknown match=%s, denying",
			       subst->auth_id, name, match);
			return axl_false;
		} /* end if */

		template = mod_sasl_mysql_get_query (node);
		query    = mod_sasl_mysql_build (auth_db_node_conf, template, subst);
		axl_free (template);
		if (query == NULL) {
			error ("Unable to build <auth-filter> [%s], denying auth for %s", name, subst->auth_id);
			return axl_false;
		} /* end if */

		msg ("Checking <auth-filter> [%s] (stage=%s, match=%s) for auth id [%s]", name, stage, match, subst->auth_id);

		if (axl_cmp (match, "expression")) {
			/* same semantics as <ip-filter> */
			allowed = mod_sasl_mysql_check_ip_filter_query (ctx, query, conn, auth_db_node_conf);
			axl_free (query);
			if (! allowed) {
				error ("login failure: %s, denied by <auth-filter> [%s] from %s",
				       subst->auth_id, name, vortex_connection_get_host_ip (conn));
				return axl_false;
			} /* end if */
			node = axl_node_get_next_called (node, "auth-filter");
			continue;
		} /* end if */

		status = mod_sasl_mysql_query_first_value (ctx, auth_db_node_conf, query, &value);
		axl_free (query);
		if (value)
			axl_free (value);

		if (status == -1) {
			error ("login failure: %s, <auth-filter> [%s] could not be evaluated, denying", subst->auth_id, name);
			return axl_false;
		} /* end if */

		if (axl_cmp (match, "required") && status != 1) {
			error ("login failure: %s, <auth-filter> [%s] reported no value and it is required", subst->auth_id, name);
			return axl_false;
		} /* end if */

		if (axl_cmp (match, "forbidden") && status == 1) {
			error ("login failure: %s, <auth-filter> [%s] reported a forbidding value", subst->auth_id, name);
			return axl_false;
		} /* end if */

		node = axl_node_get_next_called (node, "auth-filter");
	} /* end while */

	return axl_true;
}

/**
 * @internal Applies every <auth-resolve> declaration, in document
 * order, to map the credential that authenticated onto the identity the
 * session must run as.
 *
 * Each declaration receives the identity resolved so far in %e (and the
 * original credential in %u), so several of them can be chained. A
 * declaration reporting no value leaves the identity untouched, which
 * is what makes them composable: only the one that recognises the
 * credential answers.
 *
 * A statement that fails aborts the resolution and denies the auth:
 * running a session under an identity we could not confirm would be
 * worse than refusing it.
 *
 * @return axl_true when the resolution completed (with or without a
 * mapping), axl_false when it must deny the auth. On success [resolved]
 * holds a newly allocated identity, or NULL when none applied.
 */
axl_bool mod_sasl_mysql_resolve_identity (TurbulenceCtx     * ctx,
					  axlNode           * auth_db_node_conf,
					  axlDoc            * doc,
					  ModSaslMysqlSubst * subst,
					  char             ** resolved)
{
	axlNode    * node;
	const char * name;
	char       * template;
	char       * query;
	char       * value;
	char       * current = NULL;
	int          status;

	if (resolved)
		(*resolved) = NULL;

	node = axl_doc_get (doc, "/sasl-auth-db/auth-resolve");
	while (node != NULL) {

		name     = mod_sasl_mysql_node_name (node);

		template = mod_sasl_mysql_get_query (node);
		query    = mod_sasl_mysql_build (auth_db_node_conf, template, subst);
		axl_free (template);
		if (query == NULL) {
			error ("Unable to build <auth-resolve> [%s], denying auth for %s", name, subst->auth_id);
			if (current)
				axl_free (current);
			return axl_false;
		} /* end if */

		status = mod_sasl_mysql_query_first_value (ctx, auth_db_node_conf, query, &value);
		axl_free (query);

		if (status == -1) {
			error ("login failure: %s, <auth-resolve> [%s] could not be evaluated, denying", subst->auth_id, name);
			if (current)
				axl_free (current);
			return axl_false;
		} /* end if */

		if (status == 1) {
			/* this declaration recognised the credential */
			if (current)
				axl_free (current);
			current            = value;
			subst->effective_id = current;
			msg ("<auth-resolve> [%s] mapped auth id [%s] onto [%s]", name, subst->auth_id, current);
		} /* end if */

		node = axl_node_get_next_called (node, "auth-resolve");
	} /* end while */

	if (resolved)
		(*resolved) = current;
	else if (current)
		axl_free (current);

	return axl_true;
}

/**
 * @internal Runs every <auth-notify> declaration whose "on" attribute
 * matches the auth result ("ok", "failed" or "any", the default).
 *
 * Failures are reported but never change the auth result: a
 * notification that could not be delivered must not lock a user out.
 */
void mod_sasl_mysql_run_notifies (TurbulenceCtx     * ctx,
				  axlNode           * auth_db_node_conf,
				  axlDoc            * doc,
				  ModSaslMysqlSubst * subst)
{
	axlNode    * node;
	const char * name;
	char       * template;
	char       * query;
	axlError   * err = NULL;

	node = axl_doc_get (doc, "/sasl-auth-db/auth-notify");
	while (node != NULL) {

		name = mod_sasl_mysql_node_name (node);

		if (! mod_sasl_mysql_notify_applies (node, subst->status)) {
			node = axl_node_get_next_called (node, "auth-notify");
			continue;
		} /* end if */

		template = mod_sasl_mysql_get_query (node);
		query    = mod_sasl_mysql_build (auth_db_node_conf, template, subst);
		axl_free (template);
		if (query == NULL) {
			error ("Unable to build <auth-notify> [%s], skipping it", name);
			node = axl_node_get_next_called (node, "auth-notify");
			continue;
		} /* end if */

		if (! mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_true, &err)) {
			error ("Unable to run <auth-notify> [%s], error was: %s", name,
			       err ? axl_error_get (err) : "<no error>");
			if (err) {
				axl_error_free (err);
				err = NULL;
			} /* end if */
		} /* end if */
		axl_free (query);

		node = axl_node_get_next_called (node, "auth-notify");
	} /* end while */

	return;
}

axl_bool mod_sasl_mysql_do_auth (TurbulenceCtx    * ctx, 
				 VortexConnection * conn,
				 axlNode          * auth_db_node_conf,
				 const char       * auth_id,
				 const char       * authorization_id,
				 const char       * formated_password,
				 const char       * password,
				 const char       * serverName,
				 const char       * sasl_method,
				 axlError        ** err)
{
	char              * query;
	axlDoc            * doc;
	axlNode           * node;
	axl_bool            _result = axl_false;
	char              * template;
	char              * effective_id = NULL;
	ModSaslMysqlSubst   subst;

	/* NOTE: values interpolated into SQL (%u, %n, %i, %m, %p) are
	 * SQL-escaped at query build time via REPLACE_ESCAPED, so no input
	 * blacklist is applied here. The password is never interpolated
	 * into SQL (it is compared in C), so it accepts any character. */

	/* substitution set shared by every statement of this auth attempt */
	subst.auth_id          = auth_id;
	subst.serverName       = serverName;
	subst.authorization_id = authorization_id;
	subst.sasl_method      = sasl_method;
	subst.peer             = vortex_connection_get_host (conn);
	subst.status           = NULL;
	subst.effective_id     = NULL;

	/* get the auth query */
	doc  = axl_node_annotate_get (auth_db_node_conf, "mysql-conf", axl_false);
	if (doc == NULL) {
		axl_error_report (err, -1, "Found no xml document defining MySQL settings to connect to the database");
		return axl_false;
	} /* end if */

	/* check for ip filter reference */
	node  = axl_doc_get (doc, "/sasl-auth-db/ip-filter");
	if (node && HAS_ATTR (node, "query")) {
		/* ip filter defined, get query. NOTE %p (the peer address)
		 * is deliberately not provided here: the declaration is
		 * expected to report the filter, not to evaluate it */
		template = mod_sasl_mysql_get_query (node);
		subst.peer = NULL;
		query      = mod_sasl_mysql_build (auth_db_node_conf, template, &subst);
		subst.peer = vortex_connection_get_host (conn);
		axl_free (template);
		if (query == NULL) {
			error ("Unable to build ip filter query, denying connection");
			return 0;
		} /* end if */

		msg ("Checking IP filter for auth id [%s], query [%s]", auth_id, query);
		
		if (! mod_sasl_mysql_check_ip_filter_query (ctx, query, conn, auth_db_node_conf)) {
			error ("login failure: %s, ip filtered by defined expression associated to user: %s denied connection from %s", 
			       auth_id, auth_id, vortex_connection_get_host_ip (conn));
			axl_free (query);
			return 0;
		}
		msg ("IP not filtered by defined expression associated to user: %s allowed connection from %s", 
		       auth_id, vortex_connection_get_host_ip (conn));
		
		/* ip not filtered, now let the auth continue */
		axl_free (query);
	} /* end if */

	/***** GENERIC PRE-AUTH FILTERS *****/
	if (! mod_sasl_mysql_run_filters (ctx, conn, auth_db_node_conf, doc, "pre-auth", &subst))
		return 0;

	/***** ALT AUTHENTICATION *****/
	/* get alt password if defined <get-password-alt> */
	node =  axl_doc_get (doc, "/sasl-auth-db/get-password-alt");
	if (node) {
		/* get query (newly allocated, must be released) */
		query = mod_sasl_mysql_get_query (node);

		/* call to do auth operation */
		_result = __mod_sasl_mysql_prepare_query_and_auth (ctx, query, conn, auth_db_node_conf, auth_id, authorization_id,
								   formated_password, password,
								   serverName, sasl_method, axl_false, 
								   /* skip login error reporting */ axl_true, err);		
		axl_free (query);

		/* clean for cleanup node <get-password-alt-cleanup> */
		node =  axl_doc_get (doc, "/sasl-auth-db/get-password-alt-cleanup");
		if (node) {
			/* get query (newly allocated, must be released) */
			query = mod_sasl_mysql_get_query (node);

			/* call and skip getting value reported */
			if (! __mod_sasl_mysql_prepare_query_and_auth (ctx, query, conn, auth_db_node_conf, auth_id, authorization_id,
								       formated_password, password,
								       serverName, sasl_method, axl_true, 
								       /* skip login error reporting */ axl_true, err))
				error ("Cleanup query failed, please, review <get-password-alt-cleanup>..");
			axl_free (query);
			
		} /* end if */
	} /* end if */

	/**** MAIN AUTHENTICATION ****/
	/* if authentication failed, try with main table */
	if (! _result) {
		/* get the node that contains the configuration */
		node  = axl_doc_get (doc, "/sasl-auth-db/get-password");
		query = mod_sasl_mysql_get_query (node);

		/* call to do auth operation */
		_result = __mod_sasl_mysql_prepare_query_and_auth (ctx, query, conn, auth_db_node_conf, auth_id, authorization_id,
								   formated_password, password,
								   serverName, sasl_method, axl_false, 
								   /* skip login error reporting */ axl_false, err);
		axl_free (query);
	} /* end if */

	/***** GENERIC IDENTITY RESOLUTION *****/
	/* only once the credential was accepted: resolving the identity of
	 * something that did not authenticate would be meaningless */
	if (_result) {
		if (! mod_sasl_mysql_resolve_identity (ctx, auth_db_node_conf, doc, &subst, &effective_id)) {
			/* the resolution could not be completed: refuse
			 * rather than running the session under an
			 * identity we could not confirm */
			_result = axl_false;
		} else if (effective_id) {
			/* keep the credential actually presented available
			 * for auditing: <auth-log> already recorded it, but
			 * other modules may want it after the fact */
			vortex_connection_set_data_full (conn, axl_strdup (MOD_SASL_ORIGINAL_ID_KEY), axl_strdup (auth_id),
							 axl_free, axl_free);

			/* publish the resolved identity as the SASL auth id
			 * of the connection.
			 *
			 * This is what makes the resolution transparent: the
			 * whole stack above (turbulence modules, and the
			 * language bindings through vortex_sasl_get_propertie)
			 * asks for the auth id of the connection and gets the
			 * identity the session must run as, without having to
			 * know that a mapping took place. Publishing it as
			 * connection data instead would not work across the
			 * bindings, which store their own object types under
			 * that same store. */
			if (mod_sasl_mysql_resolve_sets_auth_id (doc)) {
				vortex_sasl_set_propertie (conn, VORTEX_SASL_AUTH_ID, axl_strdup (effective_id), axl_free);
				msg ("auth id [%s] resolved onto identity [%s] for conn-id=%d (published as SASL auth id)",
				     auth_id, effective_id, vortex_connection_get_id (conn));
			} else {
				msg ("auth id [%s] resolved onto identity [%s] for conn-id=%d (not published, set-auth-id=no)",
				     auth_id, effective_id, vortex_connection_get_id (conn));
			} /* end if */
		} /* end if */
	} /* end if */

	/***** GENERIC POST-AUTH FILTERS *****/
	/* evaluated with the resolved identity available in %e, so a
	 * declaration can check the identity the session will run as and
	 * not only the credential presented */
	if (_result && ! mod_sasl_mysql_run_filters (ctx, conn, auth_db_node_conf, doc, "post-auth", &subst))
		_result = axl_false;

	/* the auth result is now known: publish it so <auth-log> and every
	 * <auth-notify> declaration can report it through %t */
	subst.status = _result ? "ok" : "failed";

	/* now check for auth-log declaration to report it */
	node = axl_doc_get (doc, "/sasl-auth-db/auth-log");
	if (node) {
		/* log auth defined */
		template = mod_sasl_mysql_get_query (node);
		query    = mod_sasl_mysql_build (auth_db_node_conf, template, &subst);
		axl_free (template);
		if (query) {
			msg ("Trying to auth-log %s:%s with query string %s", auth_id, subst.status, query);
			/* exec query */
			if (! mod_sasl_mysql_do_query (ctx, auth_db_node_conf, query, axl_true, err)) {
				error ("Unable to auth-log, failed query configured, error was: %d:%s",
				       axl_error_get_code (*err), axl_error_get (*err));
				axl_error_free (*err);
			}
			axl_free (query);
		} /* end if */
		
	} /* end if */

	/***** GENERIC NOTIFICATIONS *****/
	mod_sasl_mysql_run_notifies (ctx, auth_db_node_conf, doc, &subst);

	if (effective_id)
		axl_free (effective_id);

	if (!_result)
		error ("login failure: %s, failed from: %s", auth_id, vortex_connection_get_host_ip (conn));
	
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
					  const char       * formated_password,
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
							   auth_id, authorization_id, formated_password, password, serverName, sasl_method, err));
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


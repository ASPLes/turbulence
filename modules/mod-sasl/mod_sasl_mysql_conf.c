/* mod_sasl_mysql_conf implementation: see mod_sasl_mysql_conf.h */

#include <mod_sasl_mysql_conf.h>

/* defaults applied when a declaration does not state them */
#define MOD_SASL_DEFAULT_NAME   "unnamed"
#define MOD_SASL_DEFAULT_STAGE  "pre-auth"
#define MOD_SASL_DEFAULT_MATCH  "expression"
#define MOD_SASL_DEFAULT_ON     "any"

/* a resolved identity is published as the SASL auth id unless the
 * document explicitly opts out */
#define MOD_SASL_DEFAULT_SET_AUTH_ID axl_true

/**
 * @brief Returns the SQL template declared at the provided node, with
 * XML entity references translated.
 *
 * IMPORTANT: this MUST translate entities. These attributes carry SQL,
 * SQL carries apostrophes, and any XML writer legitimately serialises
 * an apostrophe inside a single quoted attribute as &apos;. Reading the
 * attribute raw sends the literal text "&apos;" to MySQL and every
 * statement dies with error 1064.
 *
 * That is not hypothetical: on 2026-08-17 an automated tool rewrote
 * auth-db.mysql.xml through an XML serializer and, because
 * <get-password> and <ip-filter> were read raw, every user of a
 * production panel was locked out until the file was restored.
 *
 * @return newly allocated string (caller must free it), or NULL when
 * the node does not declare a query.
 */
char * mod_sasl_mysql_get_query (axlNode * node)
{
	if (node == NULL || ! HAS_ATTR (node, "query"))
		return NULL;
	return ATTR_VALUE_TRANS (node, "query");
}

/**
 * @brief Builds the final SQL statement for the provided template,
 * replacing every recognised token.
 *
 * Every value interpolated goes through [escape], with the single
 * exception of %t, which is a constant produced by the backend
 * ("ok"/"failed") and never comes from the peer. A NULL value is
 * replaced by the empty string so a template is never left carrying a
 * dangling token.
 *
 * @return newly allocated statement (caller must free it) or NULL.
 */
char * mod_sasl_mysql_build_query (const char             * query_template,
				   ModSaslMysqlSubst      * subst,
				   ModSaslMysqlEscapeFunc   escape,
				   axlPointer               escape_data)
{
	char       * query;
	char       * escaped;
	const char * tokens[6];
	const char * values[6];
	int          iterator;

	if (query_template == NULL || subst == NULL || escape == NULL)
		return NULL;

	query = axl_strdup (query_template);
	if (query == NULL)
		return NULL;

	/* %t is a controlled constant, never peer provided, so it is the
	 * only token that is not escaped */
	axl_replace (query, "%t", subst->status ? subst->status : "");

	tokens[0] = "%u"; values[0] = subst->auth_id;
	tokens[1] = "%n"; values[1] = subst->serverName;
	tokens[2] = "%i"; values[2] = subst->authorization_id;
	tokens[3] = "%m"; values[3] = subst->sasl_method;
	tokens[4] = "%p"; values[4] = subst->peer;
	tokens[5] = "%e"; values[5] = subst->effective_id;

	iterator = 0;
	while (iterator < 6) {
		escaped = escape (escape_data, values[iterator]);
		axl_replace (query, tokens[iterator], escaped ? escaped : "");
		if (escaped)
			axl_free (escaped);
		iterator++;
	} /* end while */

	return query;
}

/**
 * @brief Name declared for a node, used for reporting. Never NULL.
 */
const char * mod_sasl_mysql_node_name (axlNode * node)
{
	if (node == NULL || ! HAS_ATTR (node, "name"))
		return MOD_SASL_DEFAULT_NAME;
	return ATTR_VALUE (node, "name");
}

/**
 * @brief Stage declared for an <auth-filter>. Defaults to "pre-auth".
 */
const char * mod_sasl_mysql_filter_stage (axlNode * node)
{
	if (node == NULL || ! HAS_ATTR (node, "stage"))
		return MOD_SASL_DEFAULT_STAGE;
	return ATTR_VALUE (node, "stage");
}

/**
 * @brief Match mode declared for an <auth-filter>. Defaults to
 * "expression", which is the behaviour of the built-in <ip-filter>.
 */
const char * mod_sasl_mysql_filter_match (axlNode * node)
{
	if (node == NULL || ! HAS_ATTR (node, "match"))
		return MOD_SASL_DEFAULT_MATCH;
	return ATTR_VALUE (node, "match");
}

/**
 * @brief Result selector declared for an <auth-notify>. Defaults to
 * "any".
 */
const char * mod_sasl_mysql_notify_on (axlNode * node)
{
	if (node == NULL || ! HAS_ATTR (node, "on"))
		return MOD_SASL_DEFAULT_ON;
	return ATTR_VALUE (node, "on");
}

/**
 * @brief Reports whether the match mode is one this backend knows how
 * to evaluate.
 *
 * An unknown mode must never be treated as "no restriction": the caller
 * denies the auth instead, otherwise a typo in the configuration would
 * silently turn a filter off.
 */
axl_bool mod_sasl_mysql_match_is_known (const char * match)
{
	if (match == NULL)
		return axl_false;
	return axl_cmp (match, "expression") || axl_cmp (match, "required") || axl_cmp (match, "forbidden");
}

/**
 * @brief Reports whether the <auth-filter> node applies to the stage
 * being evaluated.
 */
axl_bool mod_sasl_mysql_filter_applies (axlNode * node, const char * stage)
{
	if (node == NULL || stage == NULL)
		return axl_false;
	return axl_cmp (mod_sasl_mysql_filter_stage (node), stage);
}

/**
 * @brief Reports whether the <auth-notify> node applies to the auth
 * result obtained.
 */
axl_bool mod_sasl_mysql_notify_applies (axlNode * node, const char * status)
{
	const char * on;

	if (node == NULL)
		return axl_false;

	on = mod_sasl_mysql_notify_on (node);
	if (axl_cmp (on, MOD_SASL_DEFAULT_ON))
		return axl_true;

	return axl_cmp (on, status ? status : "");
}


/**
 * @brief Reports whether an identity produced by <auth-resolve> must be
 * published as the SASL auth id of the connection.
 *
 * Declared at the root element, and defaulting to yes:
 *
 *   <sasl-auth-db set-auth-id="no"> ... </sasl-auth-db>
 *
 * Publishing is what makes a mapping transparent: everything above
 * mod-sasl (turbulence modules and the language bindings) asks the
 * connection for its auth id and gets the identity the session must run
 * as, without having to know that a mapping took place. That is the
 * behaviour an api-token style deployment wants, so it is the default.
 *
 * Setting it to "no" keeps the resolution available for %e, post-auth
 * filters and notifications while leaving the credential presented as
 * the auth id. That suits a deployment that resolves only to audit or
 * to drive additional checks, and does not want the session identity to
 * change underneath the rest of the stack.
 *
 * Only the exact value "no" disables it: an unrecognised value keeps
 * the default rather than silently changing which identity a session
 * runs as.
 */
axl_bool mod_sasl_mysql_resolve_sets_auth_id (axlDoc * doc)
{
	axlNode * root;

	if (doc == NULL)
		return MOD_SASL_DEFAULT_SET_AUTH_ID;

	root = axl_doc_get_root (doc);
	if (root == NULL || ! HAS_ATTR (root, "set-auth-id"))
		return MOD_SASL_DEFAULT_SET_AUTH_ID;

	return ! axl_cmp (ATTR_VALUE (root, "set-auth-id"), "no");
}

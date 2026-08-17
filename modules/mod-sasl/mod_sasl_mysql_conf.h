/* mod_sasl_mysql_conf: configuration layer of the mysql sasl backend
 *
 * This is the part of the backend that only reads and interprets the
 * declarations of auth-db.mysql.xml: which SQL template a node carries,
 * which extension declarations apply to the current step, and how the
 * recognised tokens are substituted into a statement.
 *
 * It deliberately depends on libaxl alone. No MySQL, no vortex, no
 * turbulence context. That is what makes it directly testable (see
 * test_mysql_sasl_conf.c) without a database, which matters because
 * every defect this layer had so far only showed up at runtime, on a
 * live authentication.
 */
#ifndef __MOD_SASL_MYSQL_CONF_H__
#define __MOD_SASL_MYSQL_CONF_H__

#include <axl.h>

/**
 * @brief Set of values that may be interpolated into any SQL template
 * declared at auth-db.mysql.xml.
 *
 * Keeping them in a single struct is what allows every declaration --
 * the built-in ones and the generic extension points -- to share the
 * exact same substitution semantics, instead of repeating the same
 * block of replacements once per feature (which is how they drifted
 * apart in the first place).
 */
typedef struct _ModSaslMysqlSubst {
	const char * auth_id;          /* %u */
	const char * serverName;       /* %n */
	const char * authorization_id; /* %i */
	const char * sasl_method;      /* %m */
	const char * peer;             /* %p */
	const char * status;           /* %t : "ok"/"failed", may be NULL */
	const char * effective_id;     /* %e : resolved identity, may be NULL */
} ModSaslMysqlSubst;

/**
 * @brief Handler used to escape a value before interpolating it into a
 * statement.
 *
 * The configuration layer does not know how to escape: escaping depends
 * on the live database connection (charset). The backend passes its
 * MySQL implementation, and the regression tests pass a deterministic
 * one, so the substitution logic is exercised exactly as it runs in
 * production.
 *
 * Must return a newly allocated string (released by the caller) or NULL.
 */
typedef char * (* ModSaslMysqlEscapeFunc) (axlPointer data, const char * value);

char * mod_sasl_mysql_get_query      (axlNode * node);

char * mod_sasl_mysql_build_query    (const char             * query_template,
				      ModSaslMysqlSubst      * subst,
				      ModSaslMysqlEscapeFunc   escape,
				      axlPointer               escape_data);

const char * mod_sasl_mysql_node_name   (axlNode * node);

const char * mod_sasl_mysql_filter_stage (axlNode * node);

const char * mod_sasl_mysql_filter_match (axlNode * node);

const char * mod_sasl_mysql_notify_on    (axlNode * node);

axl_bool mod_sasl_mysql_filter_applies   (axlNode * node, const char * stage);

axl_bool mod_sasl_mysql_notify_applies   (axlNode * node, const char * status);

axl_bool mod_sasl_mysql_match_is_known   (const char * match);

#endif /* __MOD_SASL_MYSQL_CONF_H__ */

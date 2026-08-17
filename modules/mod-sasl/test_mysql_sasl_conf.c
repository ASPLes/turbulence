/* Regression tests for the configuration layer of the mysql sasl
 * backend (mod_sasl_mysql_conf.c) and for the DTD that validates
 * auth-db.mysql.xml (mysql.sasl.dtd.h).
 *
 * WHY THIS EXISTS
 * ---------------
 * Every defect this backend has had showed up only at runtime, on a
 * live authentication, against a real database. There was no way to
 * exercise it without MySQL, so nothing was exercised at all. On
 * 2026-08-17 that cost a production panel every single login: an
 * automated tool rewrote auth-db.mysql.xml through an XML serializer,
 * the SQL apostrophes became &apos;, <get-password> and <ip-filter>
 * were read without translating entities, and MySQL answered 1064 to
 * every statement.
 *
 * The configuration layer was therefore split out so it depends on
 * libaxl alone: reading a template, deciding which declarations apply,
 * and substituting tokens are now pure operations that can be pinned
 * here. Escaping is injected as a handler, so the substitution logic
 * runs exactly as it does in production while the test supplies a
 * deterministic escaper.
 *
 * Build and run:   make test-mysql-sasl-conf
 * Exits non-zero on the first failing expectation.
 */

#include <stdio.h>
#include <string.h>

#include <mod_sasl_mysql_conf.h>
#include <mysql.sasl.dtd.h>

/* test bookkeeping */
int  test_passed = 0;
int  test_failed = 0;

void check (axl_bool condition, const char * description)
{
	if (condition) {
		test_passed++;
		return;
	}
	printf ("  FAIL: %s\n", description);
	test_failed++;
	return;
}

void check_str (const char * got, const char * expected, const char * description)
{
	if (got != NULL && expected != NULL && axl_cmp (got, expected)) {
		test_passed++;
		return;
	}
	printf ("  FAIL: %s\n", description);
	printf ("        expected: [%s]\n", expected ? expected : "<null>");
	printf ("        got     : [%s]\n", got ? got : "<null>");
	test_failed++;
	return;
}

/* ------------------------------------------------------------------ */
/* Escape handler used by the tests.
 *
 * It wraps the value in <> so a test can tell, unambiguously, which
 * parts of a statement went through escaping and which did not. That is
 * the property that matters: %t must NOT be escaped (it is a constant
 * produced by the backend) while every peer provided token MUST be.    */
/* ------------------------------------------------------------------ */
char * test_escape (axlPointer data, const char * value)
{
	if (value == NULL)
		return NULL;
	return axl_strdup_printf ("<%s>", value);
}

/* an escaper that always fails, to check NULL handling */
char * test_escape_null (axlPointer data, const char * value)
{
	return NULL;
}

/* ------------------------------------------------------------------ */
/* 1. Template reading: XML entities MUST be translated                */
/* ------------------------------------------------------------------ */
void test_get_query (void)
{
	axlDoc   * doc;
	axlNode  * node;
	axlError * err = NULL;
	char     * query;

	printf ("Test 01: reading SQL templates (XML entity translation)\n");

	/* the shape an XML serializer produces: single quoted attribute
	 * with the apostrophes escaped as &apos;. This is exactly the
	 * file that locked a production panel out */
	doc = axl_doc_parse ("<root><get-password query='SELECT p FROM u WHERE n = &apos;%u&apos;' /></root>", -1, &err);
	check (doc != NULL, "document with &apos; parses");
	if (doc == NULL) {
		if (err)
			axl_error_free (err);
		return;
	}
	node  = axl_doc_get (doc, "/root/get-password");
	query = mod_sasl_mysql_get_query (node);
	check_str (query, "SELECT p FROM u WHERE n = '%u'",
		   "&apos; is translated into a real apostrophe (2026-08-17 outage)");
	if (query)
		axl_free (query);
	axl_doc_free (doc);

	/* the shape core-admin ships: double quoted attribute with
	 * literal apostrophes. Must come out untouched */
	doc = axl_doc_parse ("<root><get-password query=\"SELECT p FROM u WHERE n = '%u'\" /></root>", -1, &err);
	check (doc != NULL, "document with literal apostrophes parses");
	if (doc == NULL) {
		if (err)
			axl_error_free (err);
		return;
	}
	node  = axl_doc_get (doc, "/root/get-password");
	query = mod_sasl_mysql_get_query (node);
	check_str (query, "SELECT p FROM u WHERE n = '%u'",
		   "literal apostrophes are preserved");
	if (query)
		axl_free (query);

	/* a node without query must report nothing rather than crash */
	node  = axl_doc_get (doc, "/root");
	query = mod_sasl_mysql_get_query (node);
	check (query == NULL, "node without query attribute reports NULL");
	check (mod_sasl_mysql_get_query (NULL) == NULL, "NULL node reports NULL");
	axl_doc_free (doc);

	/* other entities that may legitimately appear in SQL */
	doc = axl_doc_parse ("<root><q query='SELECT 1 WHERE a &lt; b AND c &gt; d AND e = &quot;x&quot; AND f &amp; g' /></root>", -1, &err);
	check (doc != NULL, "document with &lt; &gt; &quot; &amp; parses");
	if (doc != NULL) {
		node  = axl_doc_get (doc, "/root/q");
		query = mod_sasl_mysql_get_query (node);
		check_str (query, "SELECT 1 WHERE a < b AND c > d AND e = \"x\" AND f & g",
			   "every XML entity is translated");
		if (query)
			axl_free (query);
		axl_doc_free (doc);
	} else if (err)
		axl_error_free (err);

	return;
}

/* ------------------------------------------------------------------ */
/* 2. Token substitution                                               */
/* ------------------------------------------------------------------ */
void test_build_query (void)
{
	ModSaslMysqlSubst subst;
	char            * query;

	printf ("Test 02: token substitution\n");

	subst.auth_id          = "alice";
	subst.serverName       = "panel.example.com";
	subst.authorization_id = "alice-authz";
	subst.sasl_method      = "plain";
	subst.peer             = "10.0.0.5";
	subst.status           = "ok";
	subst.effective_id     = "real-user";

	/* every peer provided token is escaped, %t is not */
	query = mod_sasl_mysql_build_query ("u=%u n=%n i=%i m=%m p=%p t=%t e=%e", &subst, test_escape, NULL);
	check_str (query,
		   "u=<alice> n=<panel.example.com> i=<alice-authz> m=<plain> p=<10.0.0.5> t=ok e=<real-user>",
		   "every token substituted, only %t left unescaped");
	if (query)
		axl_free (query);

	/* a token appearing several times is replaced everywhere */
	query = mod_sasl_mysql_build_query ("%u and %u again", &subst, test_escape, NULL);
	check_str (query, "<alice> and <alice> again", "repeated token replaced everywhere");
	if (query)
		axl_free (query);

	/* NULL values become the empty string, never a dangling token:
	 * an unresolved %e reaching MySQL would be a syntax error */
	subst.status       = NULL;
	subst.effective_id = NULL;
	query = mod_sasl_mysql_build_query ("t=[%t] e=[%e]", &subst, test_escape, NULL);
	check_str (query, "t=[] e=[]", "NULL status and identity become empty, not a dangling token");
	if (query)
		axl_free (query);

	/* an escaper that fails must not leave the token behind either */
	subst.status = "failed";
	query = mod_sasl_mysql_build_query ("u=[%u] t=[%t]", &subst, test_escape_null, NULL);
	check_str (query, "u=[] t=[failed]", "failing escaper yields empty value, never the raw token");
	if (query)
		axl_free (query);

	/* templates with no token are returned as they are */
	query = mod_sasl_mysql_build_query ("SELECT 1", &subst, test_escape, NULL);
	check_str (query, "SELECT 1", "template without tokens is unchanged");
	if (query)
		axl_free (query);

	/* defensive arguments */
	check (mod_sasl_mysql_build_query (NULL, &subst, test_escape, NULL) == NULL, "NULL template reports NULL");
	check (mod_sasl_mysql_build_query ("x", NULL, test_escape, NULL) == NULL, "NULL substitution set reports NULL");
	check (mod_sasl_mysql_build_query ("x", &subst, NULL, NULL) == NULL, "NULL escaper reports NULL");

	return;
}

/* ------------------------------------------------------------------ */
/* 3. Which declarations apply                                         */
/* ------------------------------------------------------------------ */
void test_predicates (void)
{
	axlDoc   * doc;
	axlNode  * node;
	axlError * err = NULL;

	printf ("Test 03: declaration selection (stage, match, on)\n");

	doc = axl_doc_parse (
		"<root>"
		"<auth-filter query='q' />"
		"<auth-filter query='q' stage='post-auth' match='required' name='named' />"
		"<auth-filter query='q' match='bogus' />"
		"<auth-notify query='q' />"
		"<auth-notify query='q' on='ok' />"
		"<auth-notify query='q' on='failed' />"
		"</root>", -1, &err);
	check (doc != NULL, "declarations document parses");
	if (doc == NULL) {
		if (err)
			axl_error_free (err);
		return;
	}

	/* defaults */
	node = axl_doc_get (doc, "/root/auth-filter");
	check_str (mod_sasl_mysql_filter_stage (node), "pre-auth", "filter stage defaults to pre-auth");
	check_str (mod_sasl_mysql_filter_match (node), "expression", "filter match defaults to expression");
	check_str (mod_sasl_mysql_node_name (node), "unnamed", "name defaults to unnamed");
	check (mod_sasl_mysql_filter_applies (node, "pre-auth"), "default filter applies to pre-auth");
	check (! mod_sasl_mysql_filter_applies (node, "post-auth"), "default filter does not apply to post-auth");

	/* explicit values */
	node = axl_node_get_next_called (node, "auth-filter");
	check_str (mod_sasl_mysql_filter_stage (node), "post-auth", "declared stage is honoured");
	check_str (mod_sasl_mysql_filter_match (node), "required", "declared match is honoured");
	check_str (mod_sasl_mysql_node_name (node), "named", "declared name is honoured");
	check (mod_sasl_mysql_filter_applies (node, "post-auth"), "post-auth filter applies to post-auth");
	check (! mod_sasl_mysql_filter_applies (node, "pre-auth"), "post-auth filter does not apply to pre-auth");

	/* an unknown match must be rejected, never treated as "no
	 * restriction": a typo would silently disable the filter */
	node = axl_node_get_next_called (node, "auth-filter");
	check (! mod_sasl_mysql_match_is_known (mod_sasl_mysql_filter_match (node)), "unknown match mode is rejected");
	check (mod_sasl_mysql_match_is_known ("expression"), "expression is a known match mode");
	check (mod_sasl_mysql_match_is_known ("required"), "required is a known match mode");
	check (mod_sasl_mysql_match_is_known ("forbidden"), "forbidden is a known match mode");
	check (! mod_sasl_mysql_match_is_known (NULL), "NULL match mode is rejected");

	/* notifications */
	node = axl_doc_get (doc, "/root/auth-notify");
	check_str (mod_sasl_mysql_notify_on (node), "any", "notify selector defaults to any");
	check (mod_sasl_mysql_notify_applies (node, "ok"), "default notify fires on ok");
	check (mod_sasl_mysql_notify_applies (node, "failed"), "default notify fires on failed");
	check (mod_sasl_mysql_notify_applies (node, NULL), "default notify fires with unknown status");

	node = axl_node_get_next_called (node, "auth-notify");
	check (mod_sasl_mysql_notify_applies (node, "ok"), "on=ok fires on ok");
	check (! mod_sasl_mysql_notify_applies (node, "failed"), "on=ok does not fire on failed");
	check (! mod_sasl_mysql_notify_applies (node, NULL), "on=ok does not fire with unknown status");

	node = axl_node_get_next_called (node, "auth-notify");
	check (mod_sasl_mysql_notify_applies (node, "failed"), "on=failed fires on failed");
	check (! mod_sasl_mysql_notify_applies (node, "ok"), "on=failed does not fire on ok");

	check (! mod_sasl_mysql_filter_applies (NULL, "pre-auth"), "NULL filter node does not apply");
	check (! mod_sasl_mysql_notify_applies (NULL, "ok"), "NULL notify node does not apply");

	axl_doc_free (doc);
	return;
}

/* ------------------------------------------------------------------ */
/* 4. DTD: what auth-db.mysql.xml is allowed to declare                */
/* ------------------------------------------------------------------ */

#define DTD_HEAD \
	"<sasl-auth-db>" \
	"<connection-settings port='' host='localhost' database='d' password='p' user='u' />" \
	"<get-password query=\"SELECT password FROM user WHERE name like binary '%u'\" />"

#define DTD_TAIL "</sasl-auth-db>"

void check_dtd (axlDtd * dtd, const char * body, axl_bool should_validate, const char * description)
{
	axlDoc   * doc;
	axlError * err = NULL;
	char     * content;
	axl_bool   valid;

	content = axl_strdup_printf ("%s%s%s", DTD_HEAD, body, DTD_TAIL);
	doc     = axl_doc_parse (content, -1, &err);
	axl_free (content);

	if (doc == NULL) {
		printf ("  FAIL: %s (document did not even parse: %s)\n", description,
			err ? axl_error_get (err) : "<no error>");
		if (err)
			axl_error_free (err);
		test_failed++;
		return;
	}

	valid = axl_dtd_validate (doc, dtd, &err);
	if (err) {
		axl_error_free (err);
		err = NULL;
	}
	axl_doc_free (doc);

	check (valid == should_validate, description);
	return;
}

void test_dtd (void)
{
	axlDtd   * dtd;
	axlError * err = NULL;

	printf ("Test 04: DTD of auth-db.mysql.xml\n");

	dtd = axl_dtd_parse (MYSQL_SASL_DTD, -1, &err);
	check (dtd != NULL, "the compiled DTD parses");
	if (dtd == NULL) {
		if (err)
			axl_error_free (err);
		return;
	}

	/* BACKWARD COMPATIBILITY: the file every server has today, with
	 * no extension declared, must keep validating */
	check_dtd (dtd,
		   "<get-password-alt query=\"SELECT t FROM p WHERE n = '%u'\" />"
		   "<get-password-alt-cleanup query=\"DELETE FROM p WHERE n = '%u'\" />"
		   "<auth-log query=\"INSERT INTO auth_log (user) VALUES ('%u')\" />"
		   "<ip-filter query=\"SELECT ip_filter FROM user WHERE name like binary '%u'\" />",
		   axl_true, "the currently deployed file still validates");

	/* the minimum file */
	check_dtd (dtd, "", axl_true, "minimum file (connection-settings + get-password) validates");

	/* the new extension points */
	check_dtd (dtd, "<auth-filter query='q' />", axl_true, "a single auth-filter validates");
	check_dtd (dtd, "<auth-filter query='q' /><auth-filter query='q' stage='post-auth' match='required' name='x' />",
		   axl_true, "repeated auth-filter validates");
	check_dtd (dtd, "<auth-resolve query='q' /><auth-resolve query='q' name='x' />",
		   axl_true, "repeated auth-resolve validates");
	check_dtd (dtd, "<auth-notify query='q' /><auth-notify query='q' on='ok' name='x' />",
		   axl_true, "repeated auth-notify validates");
	check_dtd (dtd,
		   "<ip-filter query='q' />"
		   "<auth-filter query='q' />"
		   "<auth-resolve query='q' />"
		   "<auth-notify query='q' />",
		   axl_true, "built-in and extension declarations combine");

	/* what must be refused */
	check_dtd (dtd, "<auth-filter name='x' />", axl_false, "auth-filter without query is refused");
	check_dtd (dtd, "<auth-resolve name='x' />", axl_false, "auth-resolve without query is refused");
	check_dtd (dtd, "<auth-notify name='x' />", axl_false, "auth-notify without query is refused");
	check_dtd (dtd, "<not-a-known-node query='q' />", axl_false, "unknown declaration is refused");

	axl_dtd_free (dtd);
	return;
}

int main (int argc, char ** argv)
{
	printf ("** mod-sasl-mysql: configuration layer regression tests **\n\n");

	test_get_query ();
	test_build_query ();
	test_predicates ();
	test_dtd ();

	printf ("\n%d checks passed, %d failed\n", test_passed, test_failed);

	if (test_failed) {
		printf ("** FAILED **\n");
		return 1;
	}
	printf ("** OK **\n");
	return 0;
}

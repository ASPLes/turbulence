/*  test_01:  BEEP application server regression test
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software: you are wellcome to develop
 *  open source applications using this library withtout any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
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

/* include local turbulence header */
#include <turbulence.h>

/* include private turbulence headers */
#include <turbulence-ctx-private.h>

/* local include to check mod-sasl */
#include <common-sasl.h>

/* turbulence context */
TurbulenceCtx * ctx = NULL;

/* vortex context */
VortexCtx     * vortex_ctx = NULL;

axl_bool        test_common_enable_debug = axl_false;

#define	INIT_AND_RUN_CONF(conf) do {                            \
	if (! test_common_init (&vCtx, &tCtx, conf))            \
		return axl_false;                               \
                                                                \
	/* run configuration */                                 \
	if (! turbulence_run_config (tCtx)) {                   \
		printf ("Failed to run configuration with %s, try running reg test with: \n  >> VORTEX_DEBUG=1 VORTEX_DEBUG_COLOR=1 ./test_01\n", \
			conf);				        \
		return axl_false;                               \
	} /* end if */                                          \
} while (0);

int  test_01_remove_all (const char * item_stored, axlPointer user_data)
{
	/* just remove dude! */
	return axl_true;
}

/** 
 * @brief Check the turbulence db list implementation.
 * 
 * 
 * @return axl_true if the dblist implementation is ok, otherwise false is
 * returned.
 */
axl_bool  test_01 ()
{
	TurbulenceDbList * dblist;
	axlError         * err;
	axlList          * list;

	/* init turbulence db list */
	if (! turbulence_db_list_init (ctx)) {
		printf ("Unable to initialize the turbulence db-list module..\n");
		return axl_false;
	}
	
	/* test if the file exists and remote it */
	if (turbulence_file_test_v ("test_01.xml", FILE_EXISTS)) {
		/* file exist, remote it */
		if (unlink ("test_01.xml") != 0) {
			printf ("Found db list file: test_01.xml but it failed to be removed\n");
			return axl_false;
		} /* end if */
	} /* end if */
	
	/* create a new turbulence db list */
	dblist = turbulence_db_list_open (ctx, &err, "test_01.xml", NULL);
	if (dblist == NULL) {
		printf ("Failed to open db list, %s\n", axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 0) {
		printf ("Expected to find 0 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	/* add items to the list */
	if (! turbulence_db_list_add (dblist, "TEST")) {
		printf ("Expected to be able to add items to the dblist\n");
		return axl_false;
	}

	if (! turbulence_db_list_add (dblist, "TEST 2")) {
		printf ("Expected to be able to add items to the dblist\n");
		return axl_false;
	}

	if (! turbulence_db_list_add (dblist, "TEST 3")) {
		printf ("Expected to be able to add items to the dblist\n");
		return axl_false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 3) {
		printf ("Expected to find 3 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST")) {
		printf ("Expected to find an item but exist function failed..\n");
		return axl_false;
	}

	/* get the list */
	list = turbulence_db_list_get (dblist);
	if (list == NULL || axl_list_length (list) != 3) {
		printf ("Expected to a list with 3 items but it wasn't found..\n");
		return axl_false;
	}
	
	if (! axl_cmp ("TEST", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST item at the 0, but it wasn't fuond..\n");
		return axl_false;
	}
	if (! axl_cmp ("TEST 2", axl_list_get_nth (list, 1))) {
		printf ("Expected to find TEST 2 item at the 1, but it wasn't fuond..\n");
		return axl_false;
	}
	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 2))) {
		printf ("Expected to find TEST 3 item at the 2, but it wasn't fuond..\n");
		return axl_false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return axl_false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 2) {
		printf ("Expected to find 2 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST 2")) {
		printf ("Expected to find an item but exist function failed..\n");
		return axl_false;
	}

	/* get the list */
	list = turbulence_db_list_get (dblist);
	if (list == NULL || axl_list_length (list) != 2) {
		printf ("Expected to a list with 2 items but it wasn't found..\n");
		return axl_false;
	}

	if (! axl_cmp ("TEST 2", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST 2 item at the 0, but it wasn't fuond..\n");
		return axl_false;
	}
	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 1))) {
		printf ("Expected to find TEST 3 item at the 1, but it wasn't fuond..\n");
		return axl_false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST 2")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return axl_false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 1) {
		printf ("Expected to find 1 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST 3")) {
		printf ("Expected to find an item but exist function failed..\n");
		return axl_false;
	}

	/* get the list */
	list = turbulence_db_list_get (dblist);
	if (list == NULL || axl_list_length (list) != 1) {
		printf ("Expected to a list with 1 items but it wasn't found..\n");
		return axl_false;
	}

	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST 3 item at the 0, but it wasn't fuond..\n");
		return axl_false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST 3")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return axl_false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 0) {
		printf ("Expected to find 0 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	/* get the list */
	list = turbulence_db_list_get (dblist);
	if (list == NULL || axl_list_length (list) != 0) {
		printf ("Expected to find empty list but it wasn't found..\n");
		return axl_false;
	}
	axl_list_free (list);

	/* close the db list */
	turbulence_db_list_close (dblist);

	/* create a new turbulence db list */
	dblist = turbulence_db_list_open (ctx, &err, "test_01.xml", NULL);
	if (dblist == NULL) {
		printf ("Failed to open db list, %s\n", axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* add items */
	turbulence_db_list_add (dblist, "TEST");
	turbulence_db_list_add (dblist, "TEST 2");
	turbulence_db_list_add (dblist, "TEST 3");
	turbulence_db_list_add (dblist, "TEST 4");
	turbulence_db_list_add (dblist, "TEST 5");
	
	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 5) {
		printf ("Expected to find 5 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	/* remove all items */
	turbulence_db_list_remove_by_func (dblist, test_01_remove_all, NULL);

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 0) {
		printf ("Expected to find 0 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return axl_false;
	} /* end if */

	/* close the db list */
	turbulence_db_list_close (dblist);
	
	/* terminate the db list */
	turbulence_db_list_cleanup (ctx);

	/* init turbulence db list */
	if (! turbulence_db_list_init (ctx)) {
		printf ("Unable to initialize the turbulence db-list module..\n");
		return axl_false;
	}

	/* open again the list */
	dblist = turbulence_db_list_open (ctx, &err, "test_01.xml", NULL);
	if (dblist == NULL) {
		printf ("Failed to open db list, %s\n", axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* terminate the db list */
	turbulence_db_list_cleanup (ctx);
	
	return axl_true;
}

#define MATCH_AND_CHECK(_expr, string, ok)  do{				       \
	/* compile and match */                                                \
	expr = turbulence_expr_compile (ctx, _expr, NULL);                     \
	if (expr == NULL) {                                                    \
		printf ("Failed to compile expression: %s..\n", _expr);        \
		return axl_false;                                              \
	}                                                                      \
	/* now match */                                                        \
	if (ok && ! turbulence_expr_match (expr, string)) {                       \
		printf ("Expected to find proper match for value %s against expression %s..\n", \
			string, _expr);                                        \
		return axl_false;                                              \
	} else if (! ok && turbulence_expr_match (expr, string)) {	       \
		printf ("Expected to *NOT* find proper match for value %s against expression %s..\n", \
			string, _expr);                                        \
		return axl_false;                                              \
	}                                                                      \
	/* free expression */                                                  \
	turbulence_expr_free (expr);                                           \
} while(0)

/** 
 * Check regular expressions.
 */
axl_bool  test_01a () {
	
	TurbulenceExpr * expr;
	TurbulenceCtx  * ctx;

	/* init ctx */
	ctx = turbulence_ctx_new ();
	if (test_common_enable_debug) {
		turbulence_log_enable       (ctx, axl_true);
		turbulence_color_log_enable (ctx, axl_true);
		turbulence_log2_enable      (ctx, axl_true);
		turbulence_log3_enable      (ctx, axl_true);
	}

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.*", "192.168.0.10", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.*", "192.168.1.10", axl_false);
	
	/* compile and match */
	MATCH_AND_CHECK("*.wildcard.test", "test.wildcard.test", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("info.*.test", "info.wildcard.test", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("    *.wildcard.test", "test.wildcard.test", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("    *.wildcard.test", "test.1wildcard.test", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("not 192.168.0.*", "192.168.1.10", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("test.server", "test.server.child", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("test.server.*", "test.server.child", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("test.server", "parent.test.server", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("*.test.server", "parent.test.server", axl_true);

	/* compile and match */
	MATCH_AND_CHECK(".*", "", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("*", "", axl_true);

	/* compile and match */
	MATCH_AND_CHECK(".*", "any", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("*", "case", axl_true);

	/* free context */
	turbulence_ctx_free (ctx);

	return axl_true;
}

/** 
 * @brie Check misc turbulence functions.
 * 
 * 
 * @return axl_true if they succeed, othewise axl_false is returned.
 */
axl_bool  test_02 ()
{
	char * value;

	if (turbulence_file_is_fullpath ("test")) {
		printf ("Expected to find a relative path..\n");
		return axl_false;
	}

	if (turbulence_file_is_fullpath ("test/test2")) {
		printf ("Expected to find a relative path..\n");
		return axl_false;
	}

#if defined(AXL_OS_WIN32)
	if (! turbulence_file_is_fullpath ("c:/test")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}
	if (! turbulence_file_is_fullpath ("c:\\test")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}

	if (! turbulence_file_is_fullpath ("d:/test")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}
	if (! turbulence_file_is_fullpath ("c:/")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}

	if (! turbulence_file_is_fullpath ("c:\\")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}
#elif defined(AXL_OS_UNIX) 
	if (! turbulence_file_is_fullpath ("/")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}

	if (! turbulence_file_is_fullpath ("/home")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}

	if (! turbulence_file_is_fullpath ("/test")) {
		printf ("Expected to find a full path..\n");
		return axl_false;
	}
#endif

	/* check base dir and file name */
	value = turbulence_base_dir ("/test");
	if (! axl_cmp (value, "/")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("/test/value");
	if (! axl_cmp (value, "/test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("/test/value/base-value.txt");
	if (! axl_cmp (value, "/test/value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:/test/value/base-value.txt");
	if (! axl_cmp (value, "c:/test/value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test\\value\\base-value.txt");
	if (! axl_cmp (value, "c:\\test\\value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value\\value\\base-value.txt");
	if (! axl_cmp (value, "c:\\test value\\value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value \\value \\base-value.txt");
	if (! axl_cmp (value, "c:\\test value \\value ")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value \\value \\ base-value.txt");
	if (! axl_cmp (value, "c:\\test value \\value ")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	/* now check file name */
	value = turbulence_file_name ("test");
	if (! axl_cmp (value, "test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test");
	if (! axl_cmp (value, "test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test/value");
	if (! axl_cmp (value, "value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test/value/base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:/test/value/base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test\\value\\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value\\value\\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value \\value \\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value \\value \\ base-value.txt");
	if (! axl_cmp (value, " base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return axl_false;
	} /* end if */
	axl_free (value);

	/* all test ok */
	return axl_true;
}

/** 
 * @brief Allows to check the sasl backend.
 * 
 * 
 * @return axl_true if it is workin, axl_false if some error is found.
 */
axl_bool  test_03 ()
{
	/* local reference */
	SaslAuthBackend * sasl_backend;
	axlError        * err;
	axlList         * users;
	SaslUser        * user;
	VortexMutex       mutex;
	char            * serverName   = NULL;
	char            * acceptedUser = "aspl";

	/* init the mutex */
	vortex_mutex_create (&mutex);

	/* init the db list support */
	if (! turbulence_db_list_init (ctx)) {
		printf ("Unable to initialize the turbulence db-list module..\n");
		return axl_false;
	}

	/* start the sasl backend */
	if (! common_sasl_load_config (ctx, &sasl_backend, "test_03.sasl.conf", NULL,  &mutex)) {
		printf ("Unable to initialize the sasl backend..\n");
		return axl_false;
	}
	
	/* check if the default aspl user exists */
 test_03_init:
	if (! common_sasl_user_exists (sasl_backend, acceptedUser, serverName, &err, &mutex)) {
		printf ("Failed while checking if the user already exists, error found: %s....\n",
			axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	}

	/* check we don't provide axl_false positive values */
	if (common_sasl_user_exists (sasl_backend, "aspl2", serverName, &err, &mutex)) {
		printf ("It was expected to not find the user \"aspl2\" but it was found!....\n");
		return axl_false;
	}
	/* free error associated we know it happens */
	axl_error_free (err);

	/* now check passwords */
	if (! common_sasl_auth_user (sasl_backend, NULL, acceptedUser, NULL, "test", serverName, &mutex)) {
		printf ("Expected to find proper validation for aspl user\n");
		return axl_false;
	}

	/* now check passwords */
	if (common_sasl_auth_user (sasl_backend, NULL, "aspl2", NULL, "test", serverName, &mutex)) {
		printf ("Expected to a failure while validating aspl2 user\n");
		return axl_false;
	}

	/* check default methods allowed */
	if (! common_sasl_method_allowed (sasl_backend, "plain", &mutex)) {
		printf ("Expected to find \"plain\" as a proper method accepted..\n");
		return axl_false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, serverName, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return axl_false;
	}
	
	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, acceptedUser)) {
		printf ("Expected to find the user aspl..\n");
		return axl_false;
	}

	if (user->disabled) {
		printf ("Expected to find user not disabled..\n");
		return axl_false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, acceptedUser, serverName, axl_true, &mutex)) {
		printf ("failed to disable a user ..\n");
		return axl_false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, serverName, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return axl_false;
	}

	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, acceptedUser)) {
		printf ("Expected to find the user aspl..\n");
		return axl_false;
	}

	if (! user->disabled) {
		printf ("Expected to find user disabled..\n");
		return axl_false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, acceptedUser, serverName, axl_false, &mutex)) {
		printf ("failed to disable a user ..\n");
		return axl_false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, serverName, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return axl_false;
	}

	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, acceptedUser)) {
		printf ("Expected to find the user aspl..\n");
		return axl_false;
	}

	if (user->disabled) {
		printf ("Expected to find user not disabled..\n");
		return axl_false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	if (common_sasl_user_is_disabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return axl_false;
	} /* end if */

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, acceptedUser, serverName, axl_true, &mutex)) {
		printf ("failed to disable a user ..\n");
		return axl_false;
	}

	if (! common_sasl_user_is_disabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("Expected to find user disabled, but found in such state..\n");
		return axl_false;
	} /* end if */

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, acceptedUser, serverName, axl_false, &mutex)) {
		printf ("failed to enable a user ..\n");
		return axl_false;
	}

	if (common_sasl_user_is_disabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return axl_false;
	} /* end if */
	
	/* CHECK ADDING/REMOVING/CHECKING USERS ON THE FLY */
	if (! common_sasl_user_add (sasl_backend, "aspl2", "test", serverName, &mutex)) {
		printf ("Expected to add without problems user aspl2, but a failure was found..\n");
		return axl_false;
	}

	/* check new user added */
	if (common_sasl_user_is_disabled (sasl_backend, "aspl2", serverName, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return axl_false;
	} /* end if */

	/* auth the new user */
	if (! common_sasl_auth_user (sasl_backend, NULL, "aspl2", NULL, "test", serverName, &mutex)) {
		printf ("Expected to find proper validation for aspl user\n");
		return axl_false;
	}

	/* check if the user exists */
	if (! common_sasl_user_exists (sasl_backend, "aspl2", serverName, &err, &mutex)) {
		printf ("It was expected to find the user \"aspl2\" but it wasn' found!....\n");
		return axl_false;
	}

	/* now remove */
	if (! common_sasl_user_remove (sasl_backend, "aspl2", serverName, &mutex)) {
		printf ("Expected to add without problems user aspl2, but a failure was found..\n");
		return axl_false;
	}
	

	/* check if the user do not exists, now */
	if (common_sasl_user_exists (sasl_backend, "aspl2", serverName, &err, &mutex)) {
		printf ("It was expected to not find the user \"aspl2\" but it wasn' found!....\n");
		return axl_false;
	}
	axl_error_free (err);

	/* check remote admin activation support */
	if (common_sasl_is_remote_admin_enabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("It was expected to not find activated remote administration support for %s inside %s\n", 
			acceptedUser, serverName);
		return axl_false;
	} /* end if */

	/* activate remote admin */
	if (! common_sasl_enable_remote_admin (sasl_backend, acceptedUser, serverName, axl_true, &mutex)) {
		printf ("It was expected to be able to enable remote administration support for %s inside %s\n",
			acceptedUser, serverName);
		return axl_false;
	}

	/* check remote admin activation support */
	if (! common_sasl_is_remote_admin_enabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("(2) It was expected to find activated remote administration support for %s inside %s\n", 
			acceptedUser, serverName);
		return axl_false;
	} /* end if */

	/* activate remote admin */
	if (! common_sasl_enable_remote_admin (sasl_backend, acceptedUser, serverName, axl_false, &mutex)) {
		printf ("It was expected to be able to disable remote administration support for %s inside %s\n",
			acceptedUser, serverName);
		return axl_false;
	}

	/* check remote admin activation support */
	if (common_sasl_is_remote_admin_enabled (sasl_backend, acceptedUser, serverName, &mutex)) {
		printf ("It was expected to not find activated remote administration support for %s inside %s, after configuring it.\n", 
			acceptedUser, serverName);
		return axl_false;
	} /* end if */

	/* now check the backend with a particular serverName
	 * domain */
	if (serverName == NULL) {
		printf ("Test 03: checking serverName associated database support..\n");
		serverName = "www.turbulence.ws";
		acceptedUser = "aspl3";		
		goto test_03_init;
	}

	/* terminate the sasl module */
	common_sasl_free (sasl_backend);

	/* release the mutex */
	vortex_mutex_destroy (&mutex);

	/* terminate db list support (this is used by the remote
	 * administration support) */
	turbulence_db_list_cleanup (ctx);

	return axl_true;
}

/** 
 * @brief Allows to check the module support provided by
 * turbulence. It tries to load the mod-test module installed in the
 * base code.
 * 
 * 
 * @return axl_true if module support is working, otherwise, axl_false is
 * returned.
 */
axl_bool  test_04 ()
{
	TurbulenceModule * module = NULL;
	const char       * path;
	int                registered = axl_false;
	

 test_04_load_again:
	/* load the module, checking the appropiate match */
	path = "../modules/mod-test/.libs/mod-test.so";
	if (vortex_support_file_test (path, FILE_EXISTS)) {
		printf ("Test 04: found module at: %s, opening..\n", path);
		module = turbulence_module_open (ctx, path);
		goto test_04_check;
	} 

	/* load the module, checking the appropiate match */
	path = "../modules/mod-test/mod-test.so";
	if (vortex_support_file_test (path, FILE_EXISTS)) {
		printf ("Test 04: found module at: %s, opening..\n", path);
		module = turbulence_module_open (ctx, path);
		goto test_04_check;
	} 
	
	/* load the module, checking the appropiate match */
	path = "../modules/mod-test/mod-test.dll";
	if (vortex_support_file_test (path, FILE_EXISTS)) {
		printf ("Test 04: found module at: %s, opening..\n", path);
		module = turbulence_module_open (ctx, path);
		goto test_04_check;
	} 

 test_04_check:
	if (module == NULL) {
		printf ("Test 04: unable to open module, failed to execute test..\n");
		return axl_false;
	}
	
	if (! registered) {
		printf ("Test 04: freeing module..\n");
		/* close the module */
		turbulence_module_free (module);

		/* load again the module */
		registered = axl_true;
		goto test_04_load_again;
	} else {
		printf ("Test 04: registering module..\n");

		/* register the module */
		turbulence_module_register (module);
	}

	/* test ok */
	return axl_true;
}

void test_05_handler (TurbulenceMediatorObject * object)
{
	axlList * list;
	int       value;
	
	/* get the user pointer value and check its content */
	value = PTR_TO_INT (turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_USER_DATA)); 
	if (value != 5)
		return;

	/* get the list reference */
	list = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);
	if (list == NULL)
		return;
	
	/* store one item */
	axl_list_append (list, INT_TO_PTR (6));
	
	return;
}

void test_05_handler2 (TurbulenceMediatorObject * object)
{
	axlList * list;
	int       value;
	
	/* get the user pointer value and check its content */
	value = PTR_TO_INT (turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_USER_DATA)); 
	if (value != 7)
		return;

	/* get the list reference */
	list = turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA);
	if (list == NULL)
		return;
	
	/* store one item */
	axl_list_append (list, INT_TO_PTR (8));
	return;
}

void test_05_handler3 (TurbulenceMediatorObject * object)
{
	/* check value */
	int       value;
	
	/* get the user pointer value and check its content */
	value = PTR_TO_INT (turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_USER_DATA)); 
	if (value != 9)
		return;

	/* get the user pointer value and check its content */
	value = PTR_TO_INT (turbulence_mediator_object_get (object, TURBULENCE_MEDIATOR_ATTR_EVENT_DATA)); 
	if (value != 20)
		return;

	/* configure return value */
	turbulence_mediator_object_set_result (object, INT_TO_PTR (29));

	return;
}

axl_bool test_05 () {
	axlList       * list;
	axlPointer      result;
	
	/* TEST-05::1 create a context */
	TurbulenceCtx * ctx = turbulence_ctx_new ();
	turbulence_mediator_init (ctx);

	/* check here default plugs created by the system */
	if (turbulence_mediator_plug_num (ctx) != 1) {
		printf ("ERROR: expected to find one plug number created but found: %d..\n", turbulence_mediator_plug_num (ctx));
		return axl_false;
	} /* end if */

	/* create a plug */
	if (! turbulence_mediator_create_plug (ctx, "test-05", "entry",
					       axl_true, test_05_handler, NULL)) {
		printf ("ERROR: failed to create plug..\n");
		return axl_false;
	}

	/* check plugs created on the system */
	if (turbulence_mediator_plug_num (ctx) != 2) {
		printf ("ERROR: expected to find one plug number created but found: %d..\n", turbulence_mediator_plug_num (ctx));
		return axl_false;
	} /* end if */

	/* check existance */
	if (! turbulence_mediator_plug_exits (ctx, "test-05", "entry")) {
		printf ("ERROR: expected to find plugn num test-05/entry but not found..\n");
		return axl_false;
	}

	/* call to finish without uninstalling */
	turbulence_mediator_cleanup (ctx);
	turbulence_ctx_free (ctx);

	/* TEST-05::2 create a context */
	ctx = turbulence_ctx_new ();
	turbulence_mediator_init (ctx);

	/* create a plug */
	if (! turbulence_mediator_create_plug (ctx, "test-05", "entry",
					       axl_true, test_05_handler, INT_TO_PTR (5))) {
		printf ("ERROR: failed to create plug..\n");
		return axl_false;
	}

	/* push an event */
	list = axl_list_new (axl_list_equal_int, NULL);
	turbulence_mediator_push_event (ctx, "test-05", "entry", list, NULL, NULL, NULL);
	
	/* check list status */
	if (axl_list_length (list) == 0) {
		printf ("ERROR: expected to find at least one item on the list..\n");
		return axl_false;
	}

	/* check value stored */
	if (PTR_TO_INT (axl_list_get_nth (list, 0)) != 6) {
		printf ("ERROR: expected to find 6 value at list[0] but found: %d..\n",
			PTR_TO_INT (axl_list_get_nth (list, 0)));
		return axl_false;
	}

	/* now check subscribe other handler */
	if (! turbulence_mediator_subscribe (ctx, "test-05", "entry", test_05_handler2, INT_TO_PTR (7))) {
		printf ("Failed to subscribe second handler ..\n");
		return axl_false;
	}

	/* push a second event */
	turbulence_mediator_push_event (ctx, "test-05", "entry", list, NULL, NULL, NULL);

	/* check list status */
	if (axl_list_length (list) != 3) {
		printf ("ERROR: expected to find at three items on the list but found: %d..\n", axl_list_length (list));
		return axl_false;
	}

	/* check value stored */
	if (PTR_TO_INT (axl_list_get_nth (list, 2)) != 8) {
		printf ("ERROR: expected to find 8 value at list[2] but found: %d..\n",
			PTR_TO_INT (axl_list_get_nth (list, 2)));
		return axl_false;
	}

	/* register an api call */
	if (! turbulence_mediator_create_api (ctx, "test-05", "api", test_05_handler3, INT_TO_PTR (9))) {
		printf ("ERROR: failed to create api..\n");
		return axl_false;
	} /* end if */

	/* call api */
	result = turbulence_mediator_call_api (ctx, "test-05", "api", INT_TO_PTR (20), NULL, NULL, NULL);

	/* check result */
	if (PTR_TO_INT (result) != 29) {
		printf ("ERROR: expected to find 29 value but found: %d..\n", PTR_TO_INT (result));
		return axl_false;
	} /* end if */

	/* free list */
	axl_list_free (list);

	/* call to finish without uninstalling */
	turbulence_mediator_cleanup (ctx);
	turbulence_ctx_free (ctx);

	return axl_true;
}


axl_bool test_common_init (VortexCtx     ** vCtx, 
			   TurbulenceCtx ** tCtx, 
			   const char     * config)
{
	/* init vortex context and support module */
	(*vCtx) = vortex_ctx_new ();

	/* init vortex support */
	vortex_support_init ((*vCtx));

	/* create turbulence context */
	(*tCtx) = turbulence_ctx_new ();
	turbulence_ctx_set_vortex_ctx ((*tCtx), (*vCtx));

	/* init libraries */
	if (! turbulence_init ((*tCtx), (*vCtx), config)) {

		/* free turbulence ctx */
		turbulence_ctx_free ((*tCtx));
		return axl_false;
	} /* end if */

	if (test_common_enable_debug) {
		turbulence_log_enable       ((*tCtx), axl_true);
		turbulence_color_log_enable ((*tCtx), axl_true);
		turbulence_log2_enable      ((*tCtx), axl_true);
		turbulence_log3_enable      ((*tCtx), axl_true);
	}

	/* init ok */
	return axl_true;
}

void     test_common_microwait (long int microseconds)
{
	VortexAsyncQueue * queue;
	queue = vortex_async_queue_new ();
	vortex_async_queue_timedpop (queue, microseconds);
	vortex_async_queue_unref (queue);
	return;
}

axl_bool test_common_exit (VortexCtx      * vCtx,
			   TurbulenceCtx  * tCtx)
{
	/* terminate turbulence execution */
	turbulence_exit (tCtx, axl_false, axl_false);

	/* free context (the very last operation) */
	turbulence_ctx_free (tCtx);
	vortex_ctx_free (vCtx);

	return axl_false;
}

axl_bool test_06 (void) {
	TurbulenceCtx * tCtx;
	VortexCtx     * vCtx;

	/* init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "../data/turbulence.example.conf"))
		return axl_false;

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_07 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	axlList          * list;

	/* init vortex and turbulence */
	INIT_AND_RUN_CONF ("test_07.conf");

	/* check here all rules are address based flag */
	if (! tCtx->all_rules_address_based) {
		printf ("ERROR (-1) expected to find all rules address based indication but found different value..\n");
		return axl_false;
	}

	/* check current connections handled */
	list = turbulence_conn_mgr_conn_list (tCtx, VortexRoleMasterListener, NULL);
	if (axl_list_length (list) != 1) {
		printf ("ERROR (1): expected to find 1 master connection registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* connect to local host */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (2): expected to find proper connection after turbulence initialization..\n");
		return axl_false;
	} /* end if */

	/* check here connection manager */
	list = turbulence_conn_mgr_conn_list (tCtx, VortexRoleInitiator, NULL);
	if (axl_list_length (list) != 1) {
		printf ("ERROR (3): expected to find 1 client connection registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* wait a 1ms to allow turbulence registering created connections */
	test_common_microwait (30000);

	/* check all connections */
	list = turbulence_conn_mgr_conn_list (tCtx, -1, NULL);
	if (axl_list_length (list) != 3) {
		printf ("ERROR (4): expected to find 3 connections registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* check how many initiators we have */
	list = turbulence_conn_mgr_conn_list (tCtx, VortexRoleInitiator, NULL);
	if (axl_list_length (list) != 1) {
		printf ("ERROR (5): expected to find 1 connection registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* check how many initiators we have */
	list = turbulence_conn_mgr_conn_list (tCtx, VortexRoleListener, NULL);
	if (axl_list_length (list) != 1) {
		printf ("ERROR (5): expected to find 1 listener connection registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* close the connection */
	vortex_connection_close (conn);

	list = turbulence_conn_mgr_conn_list (tCtx, VortexRoleInitiator, NULL);
	if (axl_list_length (list) != 0) {
		printf ("ERROR (5): expected to find 0 connections registered, but found %d..\n", 
			axl_list_length (list));
		return axl_false;
	}
	axl_list_free (list);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

#define SIMPLE_URI_REGISTER(uri) do{                           \
	vortex_profiles_register (vCtx, uri,                   \
				  /* channel start handler */  \
				  NULL, NULL,                  \
				  /* channel close handler */  \
				  NULL, NULL,                  \
				  /* frame received handler */ \
				  NULL, NULL);                 \
	}while(0)

#define SIMPLE_CHANNEL_CREATE(uri) vortex_channel_new (conn, 0, uri, NULL, NULL, NULL, NULL, NULL, NULL)
#define SIMPLE_CHANNEL_CREATE_WITH_CONN(conn_to_use, uri) vortex_channel_new (conn_to_use, 0, uri, NULL, NULL, NULL, NULL, NULL, NULL)

axl_bool test_08 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_08.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-2");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-3");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create connection to local server */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel != NULL) {
		printf ("ERROR (2): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* now check allow profile */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-3");
	if (channel == NULL) {
		printf ("ERROR (3): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}

	/* now create profile 1 and then profile 2 */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (4): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel == NULL) {
		printf ("ERROR (5): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}

	/* check profile 4 (must not work) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-4");
	if (channel != NULL) {
		printf ("ERROR (6): expected to NULL reference after channel creation but found proper reference..\n");
		return axl_false;
	}

	/* terminate connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* SECOND PART: now check unsupported profile path for connection location */
	/* init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_08b.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-2");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-3");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* check to connect to local host */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (7): expected to not find connection ok status after turbulence setup for local area network..\n");
		return axl_false;
	} /* end if */

	/* check status */
	if (vortex_connection_get_status (conn) != VortexGreetingsFailure) {
		printf ("ERROR (8): expected to find connection status VortexGreetingsFailure (10) but found: %d:%s..\n",
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	/* close connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_09 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_09.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-3");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create connection to local server */
	printf ("Test 09: testing services provided to test-09.server domain..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-09.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel != NULL) {
		printf ("ERROR (2): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* now check profile 3: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-3");
	if (channel != NULL) {
		printf ("ERROR (3): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}

	/* now create profile 1: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (4): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}
	/* terminate connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* create connection to local server: request for test-09.second.server domain */
	printf ("Test 09: testing services provided to test-09.second.server domain..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-09.second.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (5): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel != NULL) {
		printf ("ERROR (6): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* now check profile 3: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-3");
	if (channel == NULL) {
		printf ("ERROR (7): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}

	/* now create profile 1: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel != NULL) {
		printf ("ERROR (8): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}
	/* terminate connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* create connection to local server: request for test.wilcard.com domain */
	printf ("Test 09: testing services provided to test.wilcard.com domain..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test.wildcard.com"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (9): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel != NULL) {
		printf ("ERROR (10): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* now check profile 3: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-3");
	if (channel != NULL) {
		printf ("ERROR (11): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}

	/* now create profile 1: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-4");
	if (channel == NULL) {
		printf ("ERROR (12): expected to find proper channel reference (create operation) but found NULL reference..\n");
		return axl_false;
	}
	/* terminate connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

TurbulenceCtx    * tCtxTest10 = NULL;

void test_10_received (VortexChannel    * channel, 
		       VortexConnection * connection, 
		       VortexFrame      * frame, 
		       axlPointer         user_data)
{
	TurbulenceCtx      * ctx = tCtxTest10;
	TurbulencePPathDef * ppath_selected;

	msg ("Received frame request at child (pid: %d): %s",
	     getpid (), (char*) vortex_frame_get_payload (frame));

	/* send pid reply */
	if (axl_cmp ("GET pid", (char*) vortex_frame_get_payload (frame))) 
		vortex_channel_send_rpyv (channel, vortex_frame_get_msgno (frame), "%d", getpid ());

	if (axl_cmp ("GET profile path", (char*) vortex_frame_get_payload (frame)))  {
		ppath_selected = turbulence_ppath_selected (connection);
		vortex_channel_send_rpyv (channel, vortex_frame_get_msgno (frame), "%s", turbulence_ppath_get_name (ppath_selected));
	}
	return;
}

void test_10_signal_handler (int _signal)
{
	/* marshal signal */
	turbulence_signal_received (tCtxTest10, _signal);
}

axl_bool test_10 (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10, "test_10.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-2");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-3");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* register a frame received for the remote side (child process) */
	vortex_profiles_set_received_handler (vCtx, "urn:aspl.es:beep:profiles:reg-test:profile-1", 
					      test_10_received, NULL);
	vortex_profiles_set_received_handler (vCtx, "urn:aspl.es:beep:profiles:reg-test:profile-2", 
					      test_10_received, NULL);

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10)) 
		return axl_false;

	/* install signal handling */
	turbulence_signal_install (tCtxTest10, axl_false, axl_false, axl_true, test_10_signal_handler);

	/* check process created at this point */
	if (turbulence_process_child_count (tCtxTest10) != 0) {
		printf ("ERROR (0): expected to find child process count equal to 0 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10));
		return axl_false;
	} /* end if */

	/* create connection to local server */
	printf ("Test 10: testing services provided to test-10.server domain..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (2): expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		return axl_false;
	}

	/* check connection after created it */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (4): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check process created at this point */
	if (turbulence_process_child_count (tCtxTest10) != 1) {
		printf ("ERROR (3): expected to find child process count equal to 1 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10));
		return axl_false;
	} /* end if */

	/* now check serverName status */
	if (! axl_cmp (vortex_connection_get_server_name (conn), "test-10.server")) {
		printf ("ERROR (5): expected to find server name %s, but found: %s..\n",
			"test-10.server", vortex_connection_get_server_name (conn));
		return axl_false;
	} /* end if */

	/* ask for remote pid and compare it to the current value */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "GET pid", 7, NULL)) {
		printf ("ERROR (6): expected to find remote pid request message sent successfully but found an error..\n");
		return axl_false;
	} /* end if */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (7): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	printf ("Test 10: Pid received: %s (parent pid: %d)..\n", (const char *) vortex_frame_get_payload (frame),
		getpid ());

	/* check child pid with the pid stored */
	if (! turbulence_process_child_exits (tCtxTest10, vortex_support_strtod ((const char*) vortex_frame_get_payload (frame), NULL))) {
		printf ("ERROR (8): expected to find child process %s to exist, but it wasn't found in the child list..\n",
			(const char *) vortex_frame_get_payload (frame));
		return axl_false;
	} 

	vortex_frame_unref (frame);

	/* close the connection and check child process */
	printf ("Test 10: closing connection and checking childs..\n");
	vortex_connection_close (conn);

	/* do a micro wait */
	printf ("Test 10: waiting 2 seconds for child to exit..\n");
	turbulence_sleep (tCtxTest10, 2000000);
	printf ("Test 10: done..checking child exist..\n");

	/* check child count */
	if (turbulence_process_child_count (tCtxTest10) != 0) {
		printf ("ERROR (9): expected to find child process count equal to 0 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10));
		return axl_false;
	} /* end if */

	/* create connection to local server */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (10): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 1 channel: MUST NOT WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel != NULL) {
		printf ("ERROR (11): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (12): expected to find connection in proper status but found unconnected..\n");
		return axl_false;
	}

	/* check to create profile 2 channel:  MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel == NULL) {
		printf ("ERROR (13): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (14): expected to find connection in proper status but found unconnected..\n");
		return axl_false;
	}

	/* call to get default profile path selected */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "GET profile path", 16, NULL)) {
		printf ("ERROR (15): expected to find remote pid request message sent successfully but found an error..\n");
		return axl_false;
	} /* end if */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (16): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	printf ("Test 10: Profile path received: %s..\n", (const char *) vortex_frame_get_payload (frame));
	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "default...")) {
		printf ("ERROR (17): expected to find profile path 'default...' but found %s..\n",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* release the frame */
	vortex_frame_unref (frame);
	
	/* close the connection */
	vortex_connection_close (conn);

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10);

	return axl_true;
}

axl_bool test_11 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel; 
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_11.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;


	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-11.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (2): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "content", 7, NULL)) {
		printf ("ERROR (3): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	printf ("Test 11: waiting turbulence reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (4): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	printf ("Test 11: reply received checking content..\n");
	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "profile path notified")) {
		printf ("ERROR (5): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close connection */
	vortex_connection_close (conn);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

TurbulenceCtx * test12Ctx = NULL;
void test_12_signal_received (int signal) {
	/* marshall signal */
	turbulence_signal_received (test12Ctx, signal);
}

/** 
 * @internal Common implementation for test_12 (to check mod-sasl with
 * and without child process creation).
 */
axl_bool test_12_common (VortexCtx     * vCtx, 
			 TurbulenceCtx * tCtx, 
			 int             number_of_connections, 
			 int             connections_after_close,
			 axl_bool        test_local_sasl)
{
	VortexConnection * conn;
	VortexChannel    * channel; 
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	axlList          * connList;

	/* SASL status */
	VortexStatus       status         = VortexError;
	char             * status_message = NULL;
	int                tries;

	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
	
	if (status != VortexOk) {
		printf ("ERROR (2): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	printf ("Test 12: authentication under domain test-12.server COMPLETE\n");

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (2): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "content", 7, NULL)) {
		printf ("ERROR (3): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (4): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "profile path notified")) {
		printf ("ERROR (5): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	printf ("Test 12: getting list of connections at child process..\n");

	/* now check how many registered connections are on the child
	   process side */
	if (! vortex_channel_send_msg (channel, "connections count", 17, NULL)) {
		printf ("ERROR (6): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (7): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "1")) {
		printf ("ERROR (8): expected to find connection count equal to 1 but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	printf ("Test 12: connection list ok, now finish..\n");

	/* clear frame */
	vortex_frame_unref (frame);

	/* at this point we must have 2 connections registered (conn and the master listener) */
	connList = turbulence_conn_mgr_conn_list (tCtx, -1, NULL);
	if (axl_list_length (connList) != number_of_connections) {
		printf ("ERROR (9): Expected to find registered connections equal to %d but found %d\n", 
			number_of_connections, axl_list_length (connList));
		return axl_false;
	} /* end if */
	axl_list_free (connList);

	/* close connection */
	vortex_connection_close (conn);
	printf ("Test 12: connection closed..\n");

	/* at this point we must have 1 connections registered (the master listener) */
	connList = turbulence_conn_mgr_conn_list (tCtx, -1, NULL);
	if (axl_list_length (connList) != connections_after_close) {
		printf ("ERROR (9.1): Expected to find registered connections equal to %d but found %d\n", 
			connections_after_close, axl_list_length (connList));
		return axl_false;
	} /* end if */
	axl_list_free (connList);

	/*** now connect using test-12.another-server to use another database **/
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.another-server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (10): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */	

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
	
	if (status == VortexOk) {
		printf ("ERROR (11): expected to find auth failure for aspl user under test-12.another-server, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel != NULL) {
		printf ("ERROR (12): expected to not find proper channel creation but channel was created..\n");
		return axl_false;
	}

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl2", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);

	if (status != VortexOk) {
		printf ("ERROR (13): expected to not find auth failure for aspl2 user under test-12.another-server, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (14): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	vortex_connection_close (conn);
	printf ("Test 12: connection closed (2)..\n");


	/*** now connect using test-12.another-server to use another database **/
	if (test_local_sasl) {
		printf ("Test 12: testing test-12.third-server, user defined SASL database..\n");
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.third-server"),
					   NULL, NULL);
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (15): expected to find proper connection after turbulence startup..\n");
			return axl_false;
		} /* end if */	
		
		/* enable SASL auth for current connection */
		status = VortexError;
		vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl3", NULL);
		vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
		vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
		
		if (status != VortexOk) {
			printf ("ERROR (16): expected to not find auth failure for aspl user under test-12.third-server, but error found was: (%d) %s..\n", status, status_message);
			return axl_false;
		} /* end if */
		
		printf ("Test 12: proper auth found for test-12.third-server..\n");
		
		/* now create a channel */
		channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
		if (channel == NULL) {
			printf ("ERROR (16): expected to not find proper channel creation but channel was created..\n");
			return axl_false;
		}
		
		vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
		if (! vortex_channel_send_msg (channel, "content", 7, NULL)) {
			printf ("ERROR (17): expected to send content but found error..\n");
			return axl_false;
		} /* end if */
		
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("ERROR (18): expected to find reply for get pid request...\n");
			return axl_false;
		} /* end if */
		
		if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "profile path notified for test-12.third-server")) {
			printf ("ERROR (19): expected to find 'profile path notified' but found '%s'",
				(const char*) vortex_frame_get_payload (frame));
			return axl_false;
		} /* end if */
		
		/* clear frame */
		vortex_frame_unref (frame);
		
		/* close connection */
		vortex_connection_close (conn);
		printf ("Test 12: connection closed (3)..\n");
		
		/* check child count here */
		tries = 3;
		while (axl_true) {
			printf ("Test 12: checking process count list %d..\n", turbulence_process_child_count (tCtx));
			if (turbulence_process_child_count (tCtx) != 0 && tries == 0) {
				printf ("ERROR (20): expected to find child count 0 but found %d..\n", 
					turbulence_process_child_count (tCtx));
				return axl_false;
			} else
				break;
			tries--;

			/* do a microwait to wait for childs to finish */
			test_common_microwait (1000000);
		}
	} /* end if */

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_12 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_12.conf")) 
		return axl_false;

	/* configure signal handling */
	test12Ctx = tCtx;
	turbulence_signal_install (tCtx, axl_false, axl_false, axl_true, test_12_signal_received);

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("sasl"), 
						   vortex_support_build_filename ("test_12_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* call implement common test_12 */
	return test_12_common (vCtx, tCtx, 2, 1, axl_true);
}

axl_bool test_12a (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_12a.conf")) 
		return axl_false;

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("sasl"), 
						   vortex_support_build_filename ("test_12_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* call implement common test_12 */
	return test_12_common (vCtx, tCtx, 3, 1, axl_false);
}

axl_bool test_13_common (VortexCtx * vCtx, TurbulenceCtx * tCtx, axl_bool skip_third_test) {
	VortexConnection * conn;
	VortexChannel    * channel; 
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* SASL status */
	VortexStatus       status         = VortexError;
	char             * status_message = NULL;

	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
	
	if (status != VortexOk) {
		printf ("ERROR (2): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	printf ("Test 13: authentication under domain test-13.server COMPLETE\n");

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (3): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test");
	if (channel == NULL) {
		printf ("ERROR (4): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check", 12, NULL)) {
		printf ("ERROR (5): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (6): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 1")) {
		printf ("ERROR (7): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_close (conn);

	/* --- TEST: test wrong initialization --- */
	printf ("Test 13: checking wrong initialization..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.wrong.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (8): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test");

	/* close connection */
	vortex_connection_close (conn);

	/* --- TEST: check second python app --- */
	printf ("Test 13: testing second python app..\n");
	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.another-server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (9): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl2", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
	
	if (status != VortexOk) {
		printf ("ERROR (10): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	printf ("Test 13: authentication under domain test-13.another-server COMPLETE\n");

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (11): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test-2");
	if (channel == NULL) {
		printf ("ERROR (12): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}
	/* unref the queue */
	vortex_async_queue_unref (queue);
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check-2", 14, NULL)) {
		printf ("ERROR (13): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (14): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 2")) {
		printf ("ERROR (15): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_close (conn);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	if (skip_third_test)
		return axl_true;

	/* --- TEST: check third python app --- */
	printf ("Test 13: testing third python app..\n");
	/* now open connection to localhost */
	queue = vortex_async_queue_new ();
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.third-server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (16): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl-3", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);

	
	if (status != VortexOk) {
		printf ("ERROR (17): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		return axl_false;
	} /* end if */

	printf ("Test 13: authentication under domain test-13.third-server COMPLETE\n");

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (18): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test-3");
	if (channel == NULL) {
		printf ("ERROR (19): expected to proper channel creation but a failure was found..\n");
		return axl_false;
	}
	/* unref the queue */
	vortex_async_queue_unref (queue);
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check-3", 14, NULL)) {
		printf ("ERROR (20): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (21): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 3")) {
		printf ("ERROR (22): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_close (conn);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

axl_bool test_13 (void) {
	TurbulenceCtx * tCtx;
	VortexCtx     * vCtx;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_13.conf")) 
		return axl_false;

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("sasl"), 
						   vortex_support_build_filename ("test_12_module", NULL));

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("python"), 
						   vortex_support_build_filename ("test_13_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* call to test common python functions */
	if (! test_13_common (vCtx, tCtx, axl_false))
		return axl_false;


	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_13_a (void) {
	TurbulenceCtx * tCtx;
	VortexCtx     * vCtx;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_13a.conf")) 
		return axl_false;

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("python"), 
						   vortex_support_build_filename ("test_13_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* call to test common python functions */
	if (! test_13_common (vCtx, tCtx, axl_true))
		return axl_false;


	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}


axl_bool test_14 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_14.conf")) 
		return axl_false;

	/* register profile to use */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-14");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create a connection to the local sever */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-14.server"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection but found an error..\n");
		return axl_false;
	} /* end if */

	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-14");
	if (channel == NULL) {
		printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* ok, now create a channel with a different serverName */
	channel = vortex_channel_new_full (conn, 0, "another.server.com", "urn:aspl.es:beep:profiles:reg-test:profile-14", EncodingNone, NULL, 0, 
					   /* close */
					   NULL, NULL, 
					   /* frame received */
					   NULL, NULL,
					   /* on channel created */
					   NULL, NULL);
	if (channel != NULL) {
		printf ("ERROR (3): expected to not find proper channel creation but found valid reerence..\n");
		return axl_false;
	}

	/* ok, now create a channel without signaling nothing */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-14");
	if (channel == NULL) {
		printf ("ERROR (4): expected to find proper channel creation but found an invalid reerence..\n");
		return axl_false;
	}

	/* close the connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_15_test_connection_count (VortexChannel    * channel, 
					VortexAsyncQueue * queue, 
					const char       * count_check)	
{
	VortexFrame * frame;

	/* send connection count message */
	vortex_channel_send_msg (channel, "connections-count", 17, NULL);
		
	/* wait reply */
	frame = vortex_channel_get_reply (channel, queue);

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), count_check)) {
		printf ("ERROR (4): expected to find 2 connections handled on current process, but found: %s\n",
			(const char *) vortex_frame_get_payload (frame));
		return axl_false;
	}

	vortex_frame_unref (frame);
	return axl_true;
}

axl_bool test_15a (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn, * conn2, * conn3;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	int                messages = 30;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_15.conf")) 
		return axl_false;

	/* register profile to use */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-15");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* connect to local server */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server"),
					   NULL, NULL);

	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection but found an error..\n");
		return axl_false;
	} /* end if */

	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-15");
	if (channel == NULL) {
		printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	printf ("Test 15-a: sending messages..\n");

	/* send messages */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	while (messages > 0) {
		vortex_channel_send_msg (channel, "this is a test", 14, NULL);
		
		/* wait reply */
		frame = vortex_channel_get_reply (channel, queue);
		
		/* check frame content */
		if (! axl_cmp (vortex_frame_get_payload (frame), "this is a test")) {
			printf ("ERROR (3): expected to find frame content 'this is a test' but found '%s'\n", 
				(const char *) vortex_frame_get_payload (frame));
			return axl_false;
		} /* end if */
		
		/* clear frame */
		vortex_frame_unref (frame);
		messages--;
	}

	printf ("Test 15-a: messages sent, creating second connection..\n");

	/* now create a second connection with the same serverName to
	   check that the child process created is reused */
	/* connect to local server */
	conn2 = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					    CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server"),
					    NULL, NULL);

	if (! vortex_connection_is_ok (conn2, axl_false)) {
		printf ("ERROR (4): expected to find proper connection but found an error..\n");
		return axl_false;
	} /* end if */

	channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn2, "urn:aspl.es:beep:profiles:reg-test:profile-15");
	if (channel == NULL) {
		printf ("ERROR (5): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* send a message to check how many connections are handled by
	   the remote child process */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	messages = 30;
	while (messages > 0) {
		vortex_channel_send_msg (channel, "this is a test", 14, NULL);
		
		/* wait reply */
		frame = vortex_channel_get_reply (channel, queue);
		
		/* check frame content */
		if (! axl_cmp (vortex_frame_get_payload (frame), "this is a test")) {
			printf ("ERROR (3): expected to find frame content 'this is a test' but found '%s'\n", 
				(const char *) vortex_frame_get_payload (frame));
			return axl_false;
		} /* end if */
		
		/* clear frame */
		vortex_frame_unref (frame);
		messages--;
	}

	/* check counnection count */
	if (! test_15_test_connection_count (channel, queue, "2"))
		return axl_false;

	/* create a third channel */
	channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn2, "urn:aspl.es:beep:profiles:reg-test:profile-15");
	if (channel == NULL) {
		printf ("ERROR (5): expected proper channel creation but NULL reference was found..\n");
		return axl_false;
	} /* end if */

	/* TEST: create a third connection */
	/* now create a third connection with the same serverName to
	   check that the child process created is reused */
	/* connect to local server */
	conn3 = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					    CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server"),
					    NULL, NULL);

	if (! vortex_connection_is_ok (conn3, axl_false)) {
		printf ("ERROR (6): expected to find proper connection but found an error..\n");
		return axl_false;
	} /* end if */

	channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn3, "urn:aspl.es:beep:profiles:reg-test:profile-15");
	if (channel == NULL) {
		printf ("ERROR (7): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* send a message to check how many connections are handled by
	   the remote child process */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	messages = 30;
	while (messages > 0) {
		vortex_channel_send_msg (channel, "this is a test", 14, NULL);
		
		/* wait reply */
		frame = vortex_channel_get_reply (channel, queue);
		
		/* check frame content */
		if (! axl_cmp (vortex_frame_get_payload (frame), "this is a test")) {
			printf ("ERROR (3): expected to find frame content 'this is a test' but found '%s'\n", 
				(const char *) vortex_frame_get_payload (frame));
			return axl_false;
		} /* end if */
		
		/* clear frame */
		vortex_frame_unref (frame);
		messages--;
	}

	/* check counnection count */
	if (! test_15_test_connection_count (channel, queue, "3"))
		return axl_false;

	/* create a third channel */
	channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn3, "urn:aspl.es:beep:profiles:reg-test:profile-15");
	if (channel == NULL) {
		printf ("ERROR (5): expected proper channel creation but NULL reference was found..\n");
		return axl_false;
	} /* end if */


	/* close third connection */
	vortex_connection_close (conn3);

	/* close the second connection */
	vortex_connection_close (conn2);

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* close connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

/** 
 * @brief Checks support for parsing anchillary data used for passing
 * socket descriptors between processes.
 */
axl_bool test_15 (void) {
	char * conn_status;
	axl_bool          handle_start_reply;
	int               channel_num;
	const char      * profile;
	const char      * profile_content;
	VortexEncoding    encoding;
	const char      * serverName;
	int               msg_no;
	int               seq_no;
	int               seq_no_expected;

	/* build connection status */
	conn_status = turbulence_process_connection_status_string (axl_true,
								   3, 
								   "urn:aspl.es:beep:profiles:reg-test:profile-15",
								   NULL,
								   EncodingNone,
								   "test-15.server",
								   17, 42301, 1234);
	turbulence_process_connection_recover_status (conn_status + 1, /* skip initial n used to signal the command */
						      &handle_start_reply,
						      &channel_num,
						      &profile,
						      &profile_content,
						      &encoding,
						      &serverName,
						      &msg_no,
						      &seq_no, 
						      &seq_no_expected);
	/* check data */
	if (! handle_start_reply) {
		printf ("ERROR (1): failed handle_start_reply (%d != %d) data\n", axl_true, handle_start_reply);
		return axl_false;
	}
	if (channel_num != 3) {
		printf ("ERROR (2): unexpected channel num %d != 3\n", channel_num);
		return axl_false;
	}
	if (! axl_cmp (profile, "urn:aspl.es:beep:profiles:reg-test:profile-15")) {
		printf ("ERROR (3): unexpected profile %s != urn:aspl.es:beep:profiles:reg-test:profile-15\n", profile);
		return axl_false;
	}
	if (profile_content != NULL) {
		printf ("ERROR (4): expected profile content pointing to NULL but found pointing to '%s'\n",
			profile_content);
		return axl_false;
	}
	if (encoding != EncodingNone) {
		printf ("ERROR (5): unexpected vortex encoding (%d != %d)\n", encoding, EncodingNone);
		return axl_false;
	}
	if (! axl_cmp (serverName, "test-15.server")) {
		printf ("ERROR (6): unexpected serverName (%s != 'test-15.server'\n", serverName);
		return axl_false;
	}
	if (msg_no != 17) {
		printf ("ERROR (7): unexpected msg_no value (%d != 17)\n", msg_no);
		return axl_false;
	}
	if (seq_no != 42301) {
		printf ("ERROR (8): unexpected msg_no value (%d != 42301)\n", seq_no);
		return axl_false;
	}
	if (seq_no_expected != 1234) {
		printf ("ERROR (9): unexpected msg_no value (%d != 1234)\n", seq_no_expected);
		return axl_false;
	}

	axl_free (conn_status);
	return axl_true;
}

void test_16_received (VortexChannel    * channel, 
		       VortexConnection * connection, 
		       VortexFrame      * frame, 
		       axlPointer         user_data)
{
	char * result;
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		if (axl_cmp (vortex_frame_get_payload (frame), "process-id")) {
			result = axl_strdup_printf ("%d", getpid ());
			vortex_channel_send_rpy (channel, result, strlen (result), vortex_frame_get_msgno (frame));
			axl_free (result);
			return;
		}
		/* do echo */
		vortex_channel_send_rpy (channel, 
					 vortex_frame_get_payload (frame), 
					 vortex_frame_get_payload_size (frame), 
					 vortex_frame_get_msgno (frame));
		return;
	} /* end if */

	return;
}

/** 
 * @brief Test that a server with a set of connections already handled
 * by the main process, are closed when a profile path is activated
 * and a fork operation is done.
 */
axl_bool test_16 (void) {
	
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn;
	int                connections = 30;
	VortexConnection * conns[connections];
	int                iterator;
	int                current_pid;
	int                forked_pid;
	VortexFrame      * frame;
	VortexAsyncQueue * queue;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_16.conf")) 
		return axl_false;

	/* register profile to use */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-16");

	vortex_profiles_set_received_handler (vCtx, "urn:aspl.es:beep:profiles:reg-test:profile-16", 
					      test_16_received, NULL);

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* connect to local server */
	iterator = 0;
	while (iterator < connections) {
		/* create a connection */
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-16.server"),
						   NULL, NULL);
		conns[iterator] = conn;
		
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (1): expected to find proper connection but found an error..\n");
			return axl_false;
		} /* end if */
		
		channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-16");
		if (channel == NULL) {
			printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
			return axl_false;
		}

		/* next iterator */
		iterator++;
	} /* end while */

	/* get current pid from process */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	/* send request to get process id from current server */
	vortex_channel_send_msg (channel, "process-id", 10, NULL);
	frame = vortex_channel_get_reply (channel, queue);
	current_pid = atoi ((const char *) vortex_frame_get_payload (frame));
	vortex_frame_unref (frame);

	printf ("Test 16: thread handled connections created, now create child process (current master process id=%d)..\n",
		current_pid);

	/* now create a connections that will be handled by a child process */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-16.server.child"),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection but found an error..\n");
		return axl_false;
	} /* end if */
		
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-16");
	if (channel == NULL) {
		printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* check servername process (it should be different) */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	/* send request to get process id from current server */
	vortex_channel_send_msg (channel, "process-id", 10, NULL);
	frame = vortex_channel_get_reply (channel, queue);
	forked_pid = atoi ((const char *) vortex_frame_get_payload (frame));
	vortex_frame_unref (frame);

	/* check forked pid value */
	if (current_pid == forked_pid) {
		printf ("ERROR: expected to find different pid value for forked child server (%d == %d)..\n",
			current_pid, forked_pid);
		return axl_false;
	}

	printf ("Test 16: close connection that triggered child process creation..\n");

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* now close connections created */
	vortex_connection_close (conn);

	printf ("Test 16: Close rest of connections..\n");

	iterator = 0;
	while (iterator < connections) {
		/* close the connection */
		vortex_connection_close (conns[iterator]);
		iterator++;
	} /* end while */
	
	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_17_result = axl_true;

axlPointer test_17_thread (VortexCtx * ctx)
{
	VortexConnection * conn;
	VortexChannel    * channel;
	int                iterator = 0;


	while (iterator < 4) {
		conn = vortex_connection_new_full (ctx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-17.server"),
						   NULL, NULL);
		/* check connection */
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (17.1): expected proper connection creation on iterator=%d\n", iterator);
			test_17_result = axl_false;
			return axl_false;
		}
		
		/* open a channel */
		channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-17");
		if (channel == NULL) {
			printf ("ERROR (17.2): expected to find proper channel creation but a failure was found..\n");
			test_17_result = axl_false;
			return axl_false;
		}
		
		vortex_connection_close (conn);

		/* next position */
		iterator++;
	} /* end while */

	return NULL;
}

/** 
 * @brief Test how works current profile path mechanism that allows to
 * create a child process (separate=yes) and reusing it for
 * connections to the same profile path (reuse=yes).
 */
axl_bool test_17 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn;
	int                connections = 500;
	VortexConnection * conns[connections];
	int                iterator;
	VortexThread       thread, thread2, thread3, thread4;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_17.conf")) 
		return axl_false;

	/* register profile to use */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-17");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	
	printf ("Test 17: creating %d connections with an opened channel..\n", connections);
	iterator = 0;
	while (iterator < connections) {

		/* open a connection */
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-17.server"),
						   NULL, NULL);
		/* check connection */
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (1): expected proper connection creation on iterator=%d (%d, %s)\n", iterator,
				vortex_connection_get_status (conn), vortex_connection_get_message (conn));
			return axl_false;
		}

		/* open a channel */
		channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-17");
		if (channel == NULL) {
			printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
			return axl_false;
		}

		/* setup the connection */
		conns[iterator] = conn;
		iterator++;
	} /* end while */

	/* now close all connections */
	printf ("Test 17: closing them %d connections..\n", connections);
	iterator = 0;
	while (iterator < connections) {
		/* close connection */
		vortex_connection_shutdown (conns[iterator]);
		vortex_connection_close (conns[iterator]);
		iterator++;
	}

	printf ("Test 17: creating 4 threads each one creating 4 connections with one channel ..\n");

	/* reset status */
	test_17_result = axl_true;

	/* call to create 4 threads that creates 3 connections */
	if (! vortex_thread_create (&thread, 
				    (VortexThreadFunc) test_17_thread, vCtx, VORTEX_THREAD_CONF_END)) {
		printf ("ERROR (3) Expected to create thread but failure found..\n");
		return axl_false;
	} 
	if (! vortex_thread_create (&thread2, 
				    (VortexThreadFunc) test_17_thread,
				    vCtx, VORTEX_THREAD_CONF_END)) {
		printf ("ERROR (4) Expected to create thread but failure found..\n");
		return axl_false;
	}
	if (! vortex_thread_create (&thread3, 
				    (VortexThreadFunc) test_17_thread,
				    vCtx, VORTEX_THREAD_CONF_END)) {
		printf ("ERROR (5) Expected to create thread but failure found..\n");
		return axl_false;
	}
	if (! vortex_thread_create (&thread4, 
				    (VortexThreadFunc) test_17_thread,
				    vCtx, VORTEX_THREAD_CONF_END)) {
		printf ("ERROR (6) Expected to create thread but failure found..\n");
		return axl_false;
	}

	printf ("Test 17: waiting threads to finish..\n");

	/* wait for all threads to finish */
	vortex_thread_destroy (&thread,  axl_false);
	vortex_thread_destroy (&thread2, axl_false);
	vortex_thread_destroy (&thread3, axl_false);
	vortex_thread_destroy (&thread4, axl_false);

	printf ("Test 17: ok, finishing test..\n");

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);
	
	return test_17_result;
}

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
#define run_test(function, message) do{           \
    if (function ()) {                            \
          printf ("%s [   OK   ]\n", message);    \
    } else {                                      \
          printf ("%s [ FAILED ]\n", message);    \
          return -1;                              \
    }                                             \
}while(0);

void test_with_context_init (void) {

	/* init vortex context and support module */
	vortex_ctx = vortex_ctx_new ();

	/* init vortex support */
	vortex_support_init (vortex_ctx);

	/* create turbulence context */
	ctx = turbulence_ctx_new ();
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);

	/* init module functions */
	turbulence_module_init (ctx);

	/* uncomment the following four lines to get debug */
	if (test_common_enable_debug) {
		turbulence_log_enable       (ctx, axl_true);
		turbulence_color_log_enable (ctx, axl_true);
		turbulence_log2_enable      (ctx, axl_true);
		turbulence_log3_enable      (ctx, axl_true);
	} /* end if */

	/* configure an additional path to run tests */
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-data", "../data");

	return;
}

void terminate_contexts (void) {
	/* terminate turbulence support module */
	vortex_support_cleanup (vortex_ctx);

	/* terminate module functions */
	turbulence_module_cleanup (ctx);

	/* free context */
	vortex_ctx_free (vortex_ctx);
	turbulence_ctx_free (ctx);
	
	return;
}

/** 
 * @brief General regression test to check all features inside
 * turbulence.
 */
int main (int argc, char ** argv)
{
	axl_bool disable_python_tests = axl_false;

	printf ("** test_01: Turbulence BEEP application server regression test\n");
	printf ("** Copyright (C) 2008 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: turbulence: %s \n",
		VERSION);
	printf ("**                   vortex:     %s \n",
		VORTEX_VERSION);
	printf ("**                   axl:        %s\n**\n",
		AXL_VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--debug] [--no-python]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <vortex@lists.aspl.es> Vortex/Turbulence Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
		if (axl_cmp (argv[argc], "--no-python"))
			disable_python_tests = axl_true;
		argc--;
	} /* end if */

	goto init;

	/* init context to be used on the following tests */
	test_with_context_init ();

	/* run tests */
	run_test (test_01, "Test 01: Turbulence db-list implementation");

	run_test (test_01a, "Test 01-a: Regular expressions");

	run_test (test_02, "Test 02: Turbulence misc functions");

	run_test (test_03, "Test 03: Sasl core backend (used by mod-sasl, tbc-sasl-conf)");

	run_test (test_04, "Test 04: Check module loading support");

	/* terminate context used by previous tests */
	terminate_contexts ();

	run_test (test_05, "Test 05: Check mediator API");

init:

	run_test (test_06, "Test 06: Turbulence startup and stop");

	return 0;

	run_test (test_07, "Test 07: Turbulence local connection");

	run_test (test_08, "Test 08: Turbulence profile path filtering (basic)");

	run_test (test_09, "Test 09: Turbulence profile path filtering (serverName)");

	run_test (test_10, "Test 10: Turbulence profile path filtering (child processes)");

	run_test (test_11, "Test 11: Check turbulence profile path selected");

	run_test (test_12, "Test 12: Check mod sasl (profile path selected authentication)"); 

	run_test (test_12a, "Test 12-a: Check mod sasl (profile path selected authentication, no childs)"); 

	if (! disable_python_tests) {
		run_test (test_13, "Test 13: Check mod python");
		
		run_test (test_13_a, "Test 13-a: Check mod python (same test, no childs)"); 
	} /* end if */

	run_test (test_14, "Test 14: Notify different server after profile path selected");

	run_test (test_15, "Test 15: anchillary data for socket passing");

	run_test (test_15a, "Test 15-a: Child creation with socket passing support");

	run_test (test_16, "Test 16: Connections that were working, must not be available at childs..");

	run_test (test_17, "Test 17: many connections at the same time for a profile path with separate=yes and reuse=yes");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */

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
	if (! common_sasl_load_config (ctx, &sasl_backend, "test_03.sasl.conf", &mutex)) {
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

axl_bool test_common_enable_debug = axl_false;

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
	test_common_microwait (3000);

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

axl_bool test_10 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_10.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-3");
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

/** 
 * @brief General regression test to check all features inside
 * turbulence.
 */
int main (int argc, char ** argv)
{
	printf ("** test_01: Turbulence BEEP application server regression test\n");
	printf ("** Copyright (C) 2008 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: turbulence: %s \n",
		VERSION);
	printf ("**                   vortex:     %s \n",
		VORTEX_VERSION);
	printf ("**                   axl:        %s\n**\n",
		AXL_VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--debug]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <vortex@lists.aspl.es> Vortex/Turbulence Mailing list\n**\n");

	/* init vortex context and support module */
	vortex_ctx = vortex_ctx_new ();

	/* init vortex support */
	vortex_support_init (vortex_ctx);

	/* create turbulence context */
	ctx = turbulence_ctx_new ();
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);

	/* uncomment the following four lines to get debug */
	if (argc > 1 && axl_cmp (argv[1], "--debug")) {
		test_common_enable_debug = axl_true;
		turbulence_log_enable       (ctx, axl_true);
		turbulence_color_log_enable (ctx, axl_true);
		turbulence_log2_enable      (ctx, axl_true);
		turbulence_log3_enable      (ctx, axl_true);
	} /* end if */

	/* init module functions */
	turbulence_module_init (ctx);

	/* configure an additional path to run tests */
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-data", "../data");

	/* test dblist */
	if (test_01 ()) {
		printf ("Test 01: Turbulence db-list implementation [   OK   ]\n");
	}else {
		printf ("Test 01: Turbulence db-list implementation [ FAILED ]\n");
		return -1;
	} /* end if */

	if (test_01a ()) {
		printf ("Test 01-a: Regular expressions [   OK   ]\n");
	}else {
		printf ("Test 01-a: Regular expressions [ FAILED ]\n");
		return -1;
	} /* end if */

	if (test_02 ()) {
		printf ("Test 02: Turbulence misc functions [   OK   ]\n");
	}else {
		printf ("Test 02: Turbulence misc functions [ FAILED ]\n");
		return -1;
	}

	if (test_03 ()) {
		printf ("Test 03: Sasl core backend (used by mod-sasl,tbc-sasl-conf)  [   OK   ]\n");
	} else {
		printf ("Test 03: Sasl core backend (used by mod-sasl,tbc-sasl-conf)  [ FAILED ]\n");
		return -1;
	}

	if (test_04 ()) {
		printf ("Test 04: Check module loading support  [   OK   ]\n");
	} else {
		printf ("Test 04: Check module loading support  [ FAILED ]\n");
		return -1;
	}

	if (test_05 ()) {
		printf ("Test 05: Check mediator API  [   OK   ]\n");
	} else {
		printf ("Test 05: Check mediator API  [ FAILED ]\n");
		return -1;
	}

	if (test_06 ()) {
		printf ("Test 06: Turbulence startup and stop  [   OK   ]\n");
	} else {
		printf ("Test 06: Turbulence startup and stop  [ FAILED ]\n");
		return -1;
	}

	if (test_07 ()) {
		printf ("Test 07: Turbulence local connection  [   OK   ]\n");
	} else {
		printf ("Test 07: Turbulence local connection  [ FAILED ]\n");
		return -1;
	}

	if (test_08 ()) {
		printf ("Test 08: Turbulence profile path filtering (basic)  [   OK   ]\n");
	} else {
		printf ("Test 08: Turbulence profile path filtering (basic)  [ FAILED ]\n");
		return -1;
	}

	if (test_09 ()) {
		printf ("Test 09: Turbulence profile path filtering (serverName)  [   OK   ]\n");
	} else {
		printf ("Test 09: Turbulence profile path filtering (serverName)  [ FAILED ]\n");
		return -1;
	}

	if (test_10 ()) {
		printf ("Test 10: Turbulence profile path filtering (child processes)  [   OK   ]\n");
	} else {
		printf ("Test 10: Turbulence profile path filtering (child processes)  [ FAILED ]\n");
		return -1;
	}

	/* terminate turbulence support module */
	vortex_support_cleanup (vortex_ctx);

	/* terminate module functions */
	turbulence_module_cleanup (ctx);

	/* free context */
	vortex_ctx_free (vortex_ctx);
	turbulence_ctx_free (ctx);

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */

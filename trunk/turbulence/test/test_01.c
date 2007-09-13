/*  test_01:  BEEP application server regression test
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.turbulence.ws
 */

/* include local turbulence header */
#include <turbulence.h>

/* local include to check mod-sasl */
#include <common-sasl.h>

/** 
 * @brief Check the turbulence db list implementation.
 * 
 * 
 * @return true if the dblist implementation is ok, otherwise false is
 * returned.
 */
bool test_01 ()
{
	TurbulenceDbList * dblist;
	axlError         * err;
	axlList          * list;
	
	/* test if the file exists and remote it */
	if (turbulence_file_test_v ("test_01.xml", FILE_EXISTS)) {
		/* file exist, remote it */
		if (unlink ("test_01.xml") != 0) {
			printf ("Found db list file: test_01.xml but it failed to be removed\n");
			return false;
		} /* end if */
	} /* end if */
	
	/* create a new turbulence db list */
	dblist = turbulence_db_list_open (&err, "test_01.xml", NULL);
	if (dblist == NULL) {
		printf ("Failed to open db list, %s\n", axl_error_get (err));
		axl_error_free (err);
		return false;
	} /* end if */

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 0) {
		printf ("Expected to find 0 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return false;
	} /* end if */

	/* add items to the list */
	if (! turbulence_db_list_add (dblist, "TEST")) {
		printf ("Expected to be able to add items to the dblist\n");
		return false;
	}

	if (! turbulence_db_list_add (dblist, "TEST 2")) {
		printf ("Expected to be able to add items to the dblist\n");
		return false;
	}

	if (! turbulence_db_list_add (dblist, "TEST 3")) {
		printf ("Expected to be able to add items to the dblist\n");
		return false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 3) {
		printf ("Expected to find 3 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST")) {
		printf ("Expected to find an item but exist function failed..\n");
		return false;
	}

	/* get the list */
	list = turbulence_db_list_status (dblist);
	if (list == NULL || axl_list_length (list) != 3) {
		printf ("Expected to a list with 3 items but it wasn't found..\n");
		return false;
	}
	
	if (! axl_cmp ("TEST", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST item at the 0, but it wasn't fuond..\n");
		return false;
	}
	if (! axl_cmp ("TEST 2", axl_list_get_nth (list, 1))) {
		printf ("Expected to find TEST 2 item at the 1, but it wasn't fuond..\n");
		return false;
	}
	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 2))) {
		printf ("Expected to find TEST 3 item at the 2, but it wasn't fuond..\n");
		return false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 2) {
		printf ("Expected to find 2 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST 2")) {
		printf ("Expected to find an item but exist function failed..\n");
		return false;
	}

	/* get the list */
	list = turbulence_db_list_status (dblist);
	if (list == NULL || axl_list_length (list) != 2) {
		printf ("Expected to a list with 2 items but it wasn't found..\n");
		return false;
	}

	if (! axl_cmp ("TEST 2", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST 2 item at the 0, but it wasn't fuond..\n");
		return false;
	}
	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 1))) {
		printf ("Expected to find TEST 3 item at the 1, but it wasn't fuond..\n");
		return false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST 2")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 1) {
		printf ("Expected to find 1 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return false;
	} /* end if */

	if (! turbulence_db_list_exists (dblist, "TEST 3")) {
		printf ("Expected to find an item but exist function failed..\n");
		return false;
	}

	/* get the list */
	list = turbulence_db_list_status (dblist);
	if (list == NULL || axl_list_length (list) != 1) {
		printf ("Expected to a list with 1 items but it wasn't found..\n");
		return false;
	}

	if (! axl_cmp ("TEST 3", axl_list_get_nth (list, 0))) {
		printf ("Expected to find TEST 3 item at the 0, but it wasn't fuond..\n");
		return false;
	}

	axl_list_free (list);

	/* remove items to the list */
	if (! turbulence_db_list_remove (dblist, "TEST 3")) {
		printf ("Expected to be able to remove items to the dblist\n");
		return false;
	}

	/* check the count of items inside */
	if (turbulence_db_list_count (dblist) != 0) {
		printf ("Expected to find 0 items stored in the db-list, but found: %d\n", 
			turbulence_db_list_count (dblist));
		return false;
	} /* end if */

	/* get the list */
	list = turbulence_db_list_status (dblist);
	if (list == NULL || axl_list_length (list) != 0) {
		printf ("Expected to find empty list but it wasn't found..\n");
		return false;
	}
	axl_list_free (list);

	/* close the db list */
	turbulence_db_list_close (dblist);
	
	return true;
}

/** 
 * @brie Check misc turbulence functions.
 * 
 * 
 * @return true if they succeed, othewise false is returned.
 */
bool test_02 ()
{
	char * value;

	if (turbulence_file_is_fullpath ("test")) {
		printf ("Expected to find a relative path..\n");
		return false;
	}

	if (turbulence_file_is_fullpath ("test/test2")) {
		printf ("Expected to find a relative path..\n");
		return false;
	}

#if defined(AXL_OS_WIN32)
	if (! turbulence_file_is_fullpath ("c:/test")) {
		printf ("Expected to find a full path..\n");
		return false;
	}
	if (! turbulence_file_is_fullpath ("c:\\test")) {
		printf ("Expected to find a full path..\n");
		return false;
	}

	if (! turbulence_file_is_fullpath ("d:/test")) {
		printf ("Expected to find a full path..\n");
		return false;
	}
	if (! turbulence_file_is_fullpath ("c:/")) {
		printf ("Expected to find a full path..\n");
		return false;
	}

	if (! turbulence_file_is_fullpath ("c:\\")) {
		printf ("Expected to find a full path..\n");
		return false;
	}
#elif defined(AXL_OS_UNIX) 
	if (! turbulence_file_is_fullpath ("/")) {
		printf ("Expected to find a full path..\n");
		return false;
	}

	if (! turbulence_file_is_fullpath ("/home")) {
		printf ("Expected to find a full path..\n");
		return false;
	}

	if (! turbulence_file_is_fullpath ("/test")) {
		printf ("Expected to find a full path..\n");
		return false;
	}
#endif

	/* check base dir and file name */
	value = turbulence_base_dir ("/test");
	if (! axl_cmp (value, "/")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("/test/value");
	if (! axl_cmp (value, "/test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("/test/value/base-value.txt");
	if (! axl_cmp (value, "/test/value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:/test/value/base-value.txt");
	if (! axl_cmp (value, "c:/test/value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test\\value\\base-value.txt");
	if (! axl_cmp (value, "c:\\test\\value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value\\value\\base-value.txt");
	if (! axl_cmp (value, "c:\\test value\\value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value \\value \\base-value.txt");
	if (! axl_cmp (value, "c:\\test value \\value ")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_base_dir ("c:\\test value \\value \\ base-value.txt");
	if (! axl_cmp (value, "c:\\test value \\value ")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	/* now check file name */
	value = turbulence_file_name ("test");
	if (! axl_cmp (value, "test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test");
	if (! axl_cmp (value, "test")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test/value");
	if (! axl_cmp (value, "value")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("/test/value/base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:/test/value/base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test\\value\\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value\\value\\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value \\value \\base-value.txt");
	if (! axl_cmp (value, "base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	value = turbulence_file_name ("c:\\test value \\value \\ base-value.txt");
	if (! axl_cmp (value, " base-value.txt")) {
		printf ("Expected to find a different value but found: %s\n", value);
		return false;
	} /* end if */
	axl_free (value);

	/* all test ok */
	return true;
}

/** 
 * @brief Allows to check the sasl backend.
 * 
 * 
 * @return true if it is workin, false if some error is found.
 */
bool test_03 ()
{
	/* local reference */
	SaslAuthBackend * sasl_backend;
	axlError        * err;
	axlList         * users;
	SaslUser        * user;
	VortexMutex       mutex;

	/* init the mutex */
	vortex_mutex_create (&mutex);

	/* start the sasl backend */
	if (! common_sasl_load_config (&sasl_backend, "test_03.sasl.conf", &mutex)) {
		printf ("Unable to initialize the sasl backend..\n");
		return false;
	}
	
	/* check if the default aspl user exists */
	if (! common_sasl_user_exists (sasl_backend, "aspl", NULL, &err, &mutex)) {
		printf ("Failed while checking if the user already exists, error found: %s....\n",
			axl_error_get (err));
		axl_error_free (err);
		return false;
	}

	/* check we don't provide false positive values */
	if (common_sasl_user_exists (sasl_backend, "aspl2", NULL, &err, &mutex)) {
		printf ("It was expected to not find the user \"aspl2\" but it was found!....\n");
		return false;
	}
	/* free error associated we know it happens */
	axl_error_free (err);

	/* now check passwords */
	if (! common_sasl_auth_user (sasl_backend, "aspl", NULL, "test", NULL, &mutex)) {
		printf ("Expected to find proper validation for aspl user\n");
		return false;
	}

	/* now check passwords */
	if (common_sasl_auth_user (sasl_backend, "aspl2", NULL, "test", NULL, &mutex)) {
		printf ("Expected to a failure while validating aspl2 user\n");
		return false;
	}

	/* check default methods allowed */
	if (! common_sasl_method_allowed (sasl_backend, "plain", &mutex)) {
		printf ("Expected to find \"plain\" as a proper method accepted..\n");
		return false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, NULL, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return false;
	}
	
	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, "aspl")) {
		printf ("Expected to find the user aspl..\n");
		return false;
	}

	if (user->disabled) {
		printf ("Expected to find user not disabled..\n");
		return false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, "aspl", NULL, true, &mutex)) {
		printf ("failed to disable a user ..\n");
		return false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, NULL, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return false;
	}

	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, "aspl")) {
		printf ("Expected to find the user aspl..\n");
		return false;
	}

	if (! user->disabled) {
		printf ("Expected to find user disabled..\n");
		return false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, "aspl", NULL, false, &mutex)) {
		printf ("failed to disable a user ..\n");
		return false;
	}

	/* get the list of users */
	users = common_sasl_get_users (sasl_backend, NULL, &mutex);
	if (users == NULL || axl_list_length (users) == 0) {
		printf ("Expected to find a list with one item: aspl..\n");
		return false;
	}

	/* check users in the list */
	user = axl_list_get_nth (users, 0);
	if (! axl_cmp (user->auth_id, "aspl")) {
		printf ("Expected to find the user aspl..\n");
		return false;
	}

	if (user->disabled) {
		printf ("Expected to find user not disabled..\n");
		return false;
	}

	/* dealloc the list associated */
	axl_list_free (users);

	if (common_sasl_user_is_disabled (sasl_backend, "aspl", NULL, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return false;
	} /* end if */

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, "aspl", NULL, true, &mutex)) {
		printf ("failed to disable a user ..\n");
		return false;
	}

	if (! common_sasl_user_is_disabled (sasl_backend, "aspl", NULL, &mutex)) {
		printf ("Expected to find user disabled, but found in such state..\n");
		return false;
	} /* end if */

	/* check disable function */
	if (! common_sasl_user_disable (sasl_backend, "aspl", NULL, false, &mutex)) {
		printf ("failed to enable a user ..\n");
		return false;
	}

	if (common_sasl_user_is_disabled (sasl_backend, "aspl", NULL, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return false;
	} /* end if */
	
	/* CHECK ADDING/REMOVING/CHECKING USERS ON THE FLY */
	if (! common_sasl_user_add (sasl_backend, "aspl2", "test", NULL, &mutex)) {
		printf ("Expected to add without problems user aspl2, but a failure was found..\n");
		return false;
	}

	/* check new user added */
	if (common_sasl_user_is_disabled (sasl_backend, "aspl2", NULL, &mutex)) {
		printf ("Expected to find user not disabled, but found in such state..\n");
		return false;
	} /* end if */

	/* auth the new user */
	if (! common_sasl_auth_user (sasl_backend, "aspl2", NULL, "test", NULL, &mutex)) {
		printf ("Expected to find proper validation for aspl user\n");
		return false;
	}

	/* check if the user exists */
	if (! common_sasl_user_exists (sasl_backend, "aspl2", NULL, &err, &mutex)) {
		printf ("It was expected to find the user \"aspl2\" but it wasn' found!....\n");
		return false;
	}

	/* now remove */
	if (! common_sasl_user_remove (sasl_backend, "aspl2", NULL, &mutex)) {
		printf ("Expected to add without problems user aspl2, but a failure was found..\n");
		return false;
	}
	

	/* check if the user do not exists, now */
	if (common_sasl_user_exists (sasl_backend, "aspl2", NULL, &err, &mutex)) {
		printf ("It was expected to not find the user \"aspl2\" but it wasn' found!....\n");
		return false;
	}
	axl_error_free (err);
	

	/* terminate the sasl module */
	common_sasl_free (sasl_backend);

	/* release the mutex */
	vortex_mutex_destroy (&mutex);

	return true;
}

/** 
 * @brief General regression test to check all features inside
 * turbulence.
 */
int main (int argc, char ** argv)
{
	printf ("** test_01: Turbulence BEEP application server regression test\n");
	printf ("** Copyright (C) 2007 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: turbulence: %s \n",
		VERSION);
	printf ("**                   vortex:     %s \n",
		VORTEX_VERSION);
	printf ("**                   axl:        %s\n**\n",
		AXL_VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./test_01\n**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <vortex@lists.aspl.es> Vortex/Turbulence Mailing list\n**\n");


	/* uncomment the following tree lines to get debug */
/*	turbulence_console_install_options ();
	exarg_parse (argc, argv);
	turbulence_console_process_options (); */

	/* init turbulence db list */
	turbulence_db_list_init ();

	/* test dblist */
	if (test_01 ()) {
		printf ("Test 01: Turbulence db-list implementation [   OK   ]\n");
	}else {
		printf ("Test 01: Turbulence db-list implementation [ FAILED ]\n");
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
	}else {
		printf ("Test 03: Sasl core backend (used by mod-sasl,tbc-sasl-conf)  [ FAILED ]\n");
		return -1;
	}

	/* terminate turbulence support module */
	vortex_support_cleanup ();

	/* terminate the db list */
	turbulence_db_list_cleanup ();


	/* terminate */
	return 0;
	
} /* end main */

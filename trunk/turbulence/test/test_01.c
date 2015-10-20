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

/* include local turbulence header */
#include <turbulence.h>

/* include private turbulence headers */
#include <turbulence-ctx-private.h>

/* local include to check mod-sasl */
#include <common-sasl.h>

#if defined(ENABLE_WEBSOCKET_SUPPORT)
/* support for websocket */
#include <vortex_websocket.h>
#endif

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

void show_conn_errors (VortexConnection * conn)
{
	int    code;
	char * msg;
	int    iterator = 0;

	while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
		printf ("ERROR: %d. %d:%s\n", ++iterator, code, msg);
		axl_free (msg);
	}

	return;
}

void show_next_fd (const char * label)
{
	VORTEX_SOCKET _socket = socket (AF_UNIX, SOCK_STREAM, 0);

	printf ("%s next socket available: %d\n", label, _socket);
	vortex_close_socket (_socket);
	return;
}

void test_set_run_time_path (TurbulenceCtx * tCtx, const char * runtime_datadir)
{
	/* find the first node configuring the provided system path */
	axlNode * node = axl_doc_get (tCtx->config, "/turbulence/global-settings/system-paths/path");

	while (node) {
		printf ("**** checking runtime node: %s = %s\n", ATTR_VALUE (node, "name"), ATTR_VALUE (node, "value"));
		if (HAS_ATTR_VALUE (node, "name", "runtime_datadir")) {
			printf ("***** !found attribute, update it..\n");
			/* remove and configure the new attribute */
			axl_node_remove_attribute (node, "value");
			axl_node_set_attribute (node, "value", runtime_datadir);

			printf ("**** now it has: %s = %s\n", ATTR_VALUE (node, "name"), ATTR_VALUE (node, "value"));
			return;
		}

		/* get next node */
		node = axl_node_get_next_called (node, "path");
	} /* end while */

	/* reached this point, it was configured the attribute, create a new node */
	node = axl_node_create ("path");
	axl_node_set_attribute (node, "name", "runtime-datadir");
	axl_node_set_attribute (node, "value", runtime_datadir);

	/* now add the node */
	axl_node_set_child (axl_doc_get (tCtx->config, "/turbulence/global-settings/system-paths"), node);

	return;
}

typedef struct _Test01FinishConn {
	VortexConnection * conn;
	int                pid;
	TurbulenceCtx    * ctx;
} Test01FinishConn;

axl_bool test_01_finish_channel (axlPointer _channel_num, axlPointer _channel, axlPointer user_data)
{
	VortexChannel * channel = _channel;
	if (vortex_channel_ref_count (channel) > 1) 
		printf ("Test 01: finish channel, normalizing channel ref count from %d to 1\n", 
			vortex_channel_ref_count (channel));
	while (vortex_channel_ref_count (channel) > 1) 
		vortex_channel_unref (channel);

	return axl_false; /* iterate over all channels */
}

void test_01_finish_conn (axlPointer _data)
{
	Test01FinishConn * data = _data;

	if (data->pid == vortex_getpid ()) {
		/* we are in the parent, just release data */
		axl_free (data);
		return;
	} /* end if */

	/* we are in the child, prepare context */
	__vortex_ctx_set_cleanup (CONN_CTX (data->conn));

	/* check which pid to see if we are in the child */
	printf ("Test 01 finish conn: finish connection id=%d on child, refs: %d\n",
	     vortex_connection_get_id (data->conn), vortex_connection_ref_count (data->conn));
	
	while (vortex_connection_ref_count (data->conn) > 1)
		vortex_connection_unref (data->conn, "test_01_finish_conn");

	/* normalize channel ref counting to 1 */
	vortex_connection_foreach_channel (data->conn, test_01_finish_channel, data->ctx);

	/* acquire a reference to avoid consuming references not
	 * setup */
	vortex_ctx_ref (CONN_CTX (data->conn));

	vortex_connection_shutdown (data->conn);
	vortex_connection_close (data->conn);

	axl_free (data);

	return;
}

void test_01_conn_created (VortexCtx * vCtx, VortexConnection * conn, axlPointer _tCtx)
{
	Test01FinishConn * data;

	data       = axl_new (Test01FinishConn, 1);
	data->conn = conn;
	data->pid  = vortex_getpid ();
	data->ctx  = _tCtx;

	printf ("Test 01: registering conn-id=%d, role=%d to finish\n", vortex_connection_get_id (conn), vortex_connection_get_role (conn));

	turbulence_ctx_set_data_full (
		_tCtx, 
		/* key and value */
		axl_strdup_printf ("%p", data), data,
		/* key destroy and value destroy */
		axl_free, test_01_finish_conn);
	return;
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

	if (test_common_enable_debug) {
		turbulence_log_enable       ((*tCtx), axl_true);
		turbulence_color_log_enable ((*tCtx), axl_true);
		turbulence_log2_enable      ((*tCtx), axl_true);
		turbulence_log3_enable      ((*tCtx), axl_true);
	}

	/* init libraries */
	if (! turbulence_init ((*tCtx), (*vCtx), config)) {

		/* free turbulence ctx */
		turbulence_ctx_free ((*tCtx));
		return axl_false;
	} /* end if */

	/* init ok */
	return axl_true;
}

void     test_common_microwait (long microseconds)
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
axl_bool  test_01 (void)
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

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.132,192.168.0.150", "192.168.0.132", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.132,192.168.0.150", "192.168.0.150", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.132,192.168.0.150", "192.168.0.123", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.132,192.168.0.*", "192.168.0.145", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("192.168.0.132,192.168.0.*", "192.168.1.145", axl_false);

	/* compile and match */
	MATCH_AND_CHECK(" 192.168.0.132 ,  192.168.0.150  ", "192.168.0.132", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("  192.168.0.132  ,  192.168.0.150  ", "192.168.0.150", axl_true);

	/* compile and match */
	MATCH_AND_CHECK(" 192.168.0.132  ,   192.168.0.150  ", "192.168.0.123", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("  192.168.0.132  ,  192.168.0.*   ", "192.168.0.145", axl_true);

	/* compile and match */
	MATCH_AND_CHECK("  192.168.0.132  ,  192.168.0.*  ", "192.168.1.145", axl_false);

	/* compile and match */
	MATCH_AND_CHECK("not  192.168.0.132  ,  192.168.0.*  ", "192.168.1.145", axl_true);

	/* free context */
	turbulence_ctx_free (ctx);

	return axl_true;
}

/** 
 * Check SMTP sending is working
 */
axl_bool  test_01b () {
	
	TurbulenceCtx * tCtx;
	VortexCtx     * vCtx;

	/* init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_01-b.conf"))
		return axl_false;

	/* send test mail */
	if (! turbulence_support_smtp_send (tCtx,
					    "test@aspl.es",
					    "test2@aspl.es",
					    "This is a test from test01-b regtest",
					    "This is a body content",
					    NULL,
					    NULL, NULL)) {
		printf ("ERROR (1): found failure while sending SMTP message\n");
		return axl_false;
	} /* end if */

	/* send test mail */
	if (! turbulence_support_smtp_send (tCtx,
					    "test@aspl.es",
					    "test2@aspl.es",
					    "This is a test from test01-b regtest",
					    "This is a body content",
					    NULL, 
					    "localhost", "25")) {
		printf ("ERROR (2): found failure while sending SMTP message\n");
		return axl_false;
	} /* end if */

	/* check to send file content */
	if (! turbulence_support_smtp_send (tCtx,
					    "test@aspl.es",
					    "test@aspl.es",
					    "This is a test from test01-b regtest",
					    NULL,
					    "test_01-b.conf",
					    "localhost", "25")) {
		printf ("ERROR (3): found failure while sending SMTP message from file\n");
		return axl_false;
	} /* end if */

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

/** 
 * @brie Check misc turbulence functions.
 * 
 * 
 * @return axl_true if they succeed, othewise axl_false is returned.
 */
axl_bool  test_02 (void)
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
axl_bool  test_03 (void)
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

axl_bool  test_03a (void){

	/* check password working */
	if (! common_sasl_check_crypt_password ("fIL09-h9Sa1", "$6$.gtOE.iGrioQzpR9$i4586ZHLXs65JWbh64n7GLjX3NlP5IHar8h6SNr9yiFVUHLD1KHspjQTMunlUgUTMeZj0raqgKclTKTkzi2WR0")) {
		printf ("ERROR: expected match found a failure was found..\n");
		return axl_false;
	} /* end if */

	if (! common_sasl_check_crypt_password ("fIL09-h9Sa1", "$6$ZDOQg8aRwDASQhwQ$k9yTiz7/Dh.ImmGzkxVdVmargohi/azNYMEihRIwsVJWsQlqV7XysS0JYycgMXVqVkOuC6M.bdcvn3sy..Lz01")) {
		printf ("ERROR: expected match found a failure was found..\n");
		return axl_false;
	} /* end if */

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
axl_bool  test_04 (void)
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
		if (! turbulence_module_register (module)) {
			printf ("Test 04: module not registered..\n");
		}
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


axl_bool __turbulence_get_system_id_info (TurbulenceCtx * ctx, const char * value, int * system_id, const char * path);

axl_bool test_05_a (void) {
	int value;
	TurbulenceCtx * ctx = turbulence_ctx_new ();
	/* turbulence_log_enable       (ctx, axl_true);
	   turbulence_color_log_enable (ctx, axl_true); */

	/* check to load and check user ids */
	if (! __turbulence_get_system_id_info (ctx, "libuuid", &value, "test_05_a_passwd"))
		return axl_false;
	
	/* check value returned */
	if (value != 100) {
		printf ("ERROR (1): expected to find 100 but found %d for user %s\n",
			value, "libuuid");
		return axl_false;
	}

	/* check to load and check user ids */
	if (! __turbulence_get_system_id_info (ctx, "backup", &value, "test_05_a_passwd"))
		return axl_false;
	
	/* check value returned */
	if (value != 34) {
		printf ("ERROR (2): expected to find 34 but found %d for user %s\n",
			value, "backup");
		return axl_false;
	}

	/* check to load and check user ids */
	if (! __turbulence_get_system_id_info (ctx, "mysql", &value, "test_05_a_passwd"))
		return axl_false;
	
	/* check value returned */
	if (value != 109) {
		printf ("ERROR (3): expected to find 109 but found %d for user %s\n",
			value, "mysql");
		return axl_false;
	}

	turbulence_ctx_free (ctx);

	return axl_true;
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

axl_bool test_06_a (void) {
	TurbulenceCtx * tCtx;
	VortexCtx     * vCtx;
	axlNode       * node;
	int             iterator;

	/* init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_06a.conf"))
		return axl_false;

	/* check that indeed data was inserted */
	node = axl_doc_get (tCtx->config, "/turbulence/global-settings/ports/port");
	if (node == NULL) {
		printf ("ERROR: expected to find ports/port node but found NULL\n");
		return axl_false;
	}

	/* now check no include is found in the document */
	if (axl_doc_find_called (tCtx->config, "include")) {
		printf ("ERROR: no include node should be found, but it was..\n");
		return axl_false;
	}

	/* check we have loaded all profile paths */
	if (! axl_doc_find_called (tCtx->config, "path-def")) {
		printf ("ERROR: checking that configuration includes a <path-def> node...but it wasn't found..\n");
		return axl_false;
	}

	/* check the number or profile path defs */
	node = axl_doc_find_called (tCtx->config, "path-def");

	iterator = 0;
	while (node) {
		/* node found */
		iterator++;

		/* next node */
		node = axl_node_get_next_called (node, "path-def");
	}

	/* check number of profile paths */
	if (iterator != 2) {
		printf ("ERROR: expected to find 2 <path-def> nodes but found %d..\n", iterator);
		return axl_false;
	}
	printf ("Test 06-a: <path-def> nodes found: %d\n", iterator);
	

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

	/* wait a 30ms to allow turbulence registering created connections */
	test_common_microwait (30000);

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
	int                code;
	char             * msg;
	axl_bool           found;

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
	found = axl_false;
	while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
		printf ("Test 08: found channel code error: %d\n", code);
		if (code == VortexGreetingsFailure) {
			found = axl_true;
			axl_free (msg);
			break;
		}
		axl_free (msg);
	} /* end if */

	if (! found) {
		printf ("ERROR (8): expected to find VortexGreetingsFailure (%d) but it wasn't found..\n",
			code);
		return axl_false;
	}

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-09.server", VORTEX_OPTS_END),
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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-09.second.server", VORTEX_OPTS_END),
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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test.wildcard.com", VORTEX_OPTS_END),
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
TurbulenceCtx    * tCtxTest10a = NULL;

void test_10_signal_handler (int _signal)
{
	/* marshal signal */
	turbulence_signal_received (tCtxTest10, _signal);
}

void test_10_a_signal_handler (int _signal)
{
	/* marshal signal */
	turbulence_signal_received (tCtxTest10a, _signal);
}

axl_bool test_10 (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	int                iterator;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10, "test_10.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10)) 
		return axl_false;

	/* create queue and release it on vortex ctx finish */
	queue = vortex_async_queue_new ();

	/* install signal handling */
	turbulence_signal_install (tCtxTest10, axl_false, axl_false, test_10_signal_handler);

	/* check process created at this point */
	if (turbulence_process_child_count (tCtxTest10) != 0) {
		printf ("ERROR (0): expected to find child process count equal to 0 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10));
		return axl_false;
	} /* end if */

	/* create connection to local server */
	printf ("Test 10: testing services provided to test-10.server domain..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST WORK */
	printf ("Test 10: creating channel..\n");
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
	printf ("Test 10: ...all checks ok, getting pid..\n");
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
	if (! turbulence_process_child_exists (tCtxTest10, vortex_support_strtod ((const char*) vortex_frame_get_payload (frame), NULL))) {
		printf ("ERROR (8): expected to find child process %s to exist, but it wasn't found in the child list..\n",
			(const char *) vortex_frame_get_payload (frame));
		return axl_false;
	} 

	vortex_frame_unref (frame);

	/* close the connection and check child process */
	printf ("Test 10: closing connection and checking childs (parent pid: %d)..\n", getpid ());
	vortex_connection_close (conn);

	/* do a micro wait */
	printf ("Test 10: waiting 4 seconds for child to exit..\n");
	iterator = 0;
	while (iterator < 4000) {
		
		/* wait a bit */
		turbulence_sleep (tCtxTest10, 1000);

		/* check child count */
		if (turbulence_process_child_count (tCtxTest10) == 0) 
			break;

		/* next */
		iterator++;
	}

	printf ("Test 10: child count after wait: %d..\n", turbulence_process_child_count (tCtxTest10));

	/* check child count */
	if (turbulence_process_child_count (tCtxTest10) != 0)  {
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

	/* wait connection to be closed */
	printf ("Test 10: waiting child process to close our connection (1 seg)..\n");
	turbulence_sleep (tCtxTest10, 1000000);
	printf ("Test 10:   wait finished (1 seg)..\n");

	/* check connection status */
	if (vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (12.10): expected to NOT find connection in proper status but found connected..\n");
		return axl_false;
	}
	vortex_connection_close (conn);

	/* create connection to local server */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (12.1.10): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check connection status */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (12.2.10): expected to find connection in proper status but found unconnected..\n");
		return axl_false;
	}

	/* check to create profile 2 channel:  MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	if (channel == NULL) {
		printf ("ERROR (13.10): expected to fidn proper channel creation but found NULL reference..\n");
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
		printf ("ERROR (16.10): expected to find reply for get value for profile path (queue items %d)...\n",
			vortex_async_queue_items (queue));
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	printf ("Test 10: Profile path received: %s..\n", (const char *) vortex_frame_get_payload (frame));
	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "default...")) {
		printf ("ERROR (17): expected to find profile path 'default...' but found %s..\n",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* release the frame */
	vortex_frame_unref (frame);
	
	/* close the connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10);

	return axl_true;
}

TurbulenceCtx * tCtxTest10prev = NULL;

void test_10_prev_signal_handler (int _signal)
{
	/* marshal signal */
	turbulence_signal_received (tCtxTest10prev, _signal);
}

axl_bool test_10_prev (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	axlList          * childs;
	int                iterator;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10prev, "test_10.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10prev)) 
		return axl_false;

	/* install signal handling (handle child processes) */
	turbulence_signal_install (tCtxTest10prev, axl_false, axl_false, test_10_prev_signal_handler);

	/* create connection to local server */
	printf ("Test 10-prev: creating child process..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	printf ("Test 10-prev: connection id=%d (%p) created..\n", 
		vortex_connection_get_id (conn), conn);

	/* check to create profile 2 channel: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (2): expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		show_conn_errors (conn);
		return axl_false;
	}

	printf ("Test 10-prev: channel num=%d (%p) created..\n", 
		vortex_channel_get_number (channel), channel);

	/* check connection after created it */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (4): expected to find proper connection after turbulence startup..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* now wait a bit 100ms */
	turbulence_sleep (tCtxTest10prev, 100000);

	/* check child count */
	if (turbulence_process_child_count (tCtxTest10prev) != 1) {
		printf ("ERROR (5): expected to find child process count equal to 1 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10prev));
		return axl_false;
	} /* end if */

	/* get child list */
	iterator = 0;
	while (iterator < 5) {
		childs = turbulence_process_child_list (tCtxTest10prev);
		if (axl_list_length (childs) != 1) {
			printf ("ERROR (5.1): expected to find child process count equal to 1 but found: %d..\n",
				axl_list_length (childs));
			return axl_false;
		}

		iterator++;

		/* release list */
		axl_list_free (childs);
	} /* end while */

	/* close connection to force child stop operation */
	printf ("Test 10-prev: closing connection to child, this should cause child to finish (pid: %d)..\n", getpid ());
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* now wait a bit 4seg */
	iterator = 0;
	while (iterator < 4000) {
		
		/* wait a bit */
		turbulence_sleep (tCtxTest10prev, 1000);

		/* check child count */
		if (turbulence_process_child_count (tCtxTest10prev) == 0) 
			break;

		/* next */
		iterator++;
	}

	printf ("Test 10-prev: child should have finished..\n");

	/* check child count */
	if (turbulence_process_child_count (tCtxTest10prev) != 0) {
		printf ("ERROR (6): expected to find child process count equal to 0 but found: %d..\n",
			turbulence_process_child_count (tCtxTest10prev));
		return axl_false;
	} /* end if */

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10prev);

	return axl_true;
}

TurbulenceChild * test_10_b_get_first_child (TurbulenceCtx * ctx)
{
	axlHashCursor   * cursor;
	TurbulenceChild * result;

	printf ("Test 10-b: child process hash reference is: %p\n", ctx->child_process);
	cursor = axl_hash_cursor_new (ctx->child_process);
	axl_hash_cursor_first (cursor);
	
	result = axl_hash_cursor_get_value (cursor);
	axl_hash_cursor_free (cursor);

	return result;
}

axl_bool test_10_b (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	TurbulenceChild  * child;
	VortexFrame      * frame;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10prev, "test_10b.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10prev)) 
		return axl_false;

	/* install signal handling (handle child processes) */
	turbulence_signal_install (tCtxTest10prev, axl_false, axl_false, test_10_prev_signal_handler);

	/* create connection to local server */
	printf ("Test 10-b: creating child process..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	printf ("Test 10-b: connection id=%d (%p) created..\n", 
		vortex_connection_get_id (conn), conn);

	/* check to create profile 2 channel: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (2): expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		return axl_false;
	}

	printf ("Test 10-b: channel num=%d (%p) created..\n", 
		vortex_channel_get_number (channel), channel);

	/* check connection after created it */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (4): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* now wait a bit 100ms */
	turbulence_sleep (tCtxTest10prev, 100000);

	/* ok, now get reference to the first child */
	child = test_10_b_get_first_child (tCtxTest10prev);
	printf ("Test 10-b: found child reference: %p\n", child);
	
	if (child == NULL) {
		printf ("Test 10-b: unable to find first child process (null reference received), unable to check master link\n");
		return axl_false;
	} /* end if */

	/* check connection and role */
	if (! vortex_connection_is_ok (child->conn_mgr, axl_false)) {
		printf ("Test 10-b: expected to find connection management (master<->child connection) but found error..\n");
		return axl_false;
	}

	/* check connection role */
	if (vortex_connection_get_role (child->conn_mgr) != VortexRoleListener) {
		printf ("Test 10-b: expected to find listener role but found: %d (%d)\n", 
			vortex_connection_get_role (child->conn_mgr), VortexRoleMasterListener);
		return axl_false;
	}
	printf ("Test 10-b: connection management at parent ok..\n");

	/* now check I can't create channels with unregistered
	 * profiles to child process */
	if (vortex_channel_new (child->conn_mgr, 0, "unsupported-channel-profile", NULL, NULL, NULL, NULL, NULL, NULL)) {
		printf ("Test 10-b: expected to not be able to create channel with child process through conn mgr, with unsupported profile..\n");
		return axl_false;
	}
	printf ("Test 10-b: default security from parent ok..\n");

	/* check child->conn_mgr is not registered at the turbulence conn manager */
	if (turbulence_conn_mgr_find_by_id (tCtxTest10prev, vortex_connection_get_id (child->conn_mgr))) {
		printf ("Test 10-b: expected to not find child->conn_mgr registered at turbulence conn manager..\n");
		return axl_false;
	}
	printf ("Test 10-b: checked child->conn_mgr is not registered at turbulence conn mgr..ok\n");

	/* ok, now ask the child to check its side */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-10b");
	if (channel == NULL) {
		printf ("ERROR (2): expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		return axl_false;
	}

	printf ("Test 10-b: child connection check channel num=%d (%p) created..\n", 
		vortex_channel_get_number (channel), channel);	

	/* now send message */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	
	/* send message */
	if (! vortex_channel_send_msg (channel, "check conn mgr", 14, NULL)) {
		printf ("ERROR (3): expected to send check conn mgr but found a failure..\n");
		return axl_false;
	} /* end if */

	/* wait reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (16): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	/* check reply type */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ERR) {
		printf ("ERROR (18): found error at child conn mgr check, error was: %s\n", 
			(const char *) vortex_frame_get_payload (frame));
		return axl_false;
	}
	
	printf ("Test 10-b: found conn check at child ok: %s\n", 
		(const char *) vortex_frame_get_payload (frame));

	/* free frame */
	vortex_frame_unref (frame);

	/* ok, now use master<->child link */
	printf ("Test 10-b: checking master<->child to create internal channels, role: %d\n", vortex_connection_get_role (child->conn_mgr));
	channel = vortex_channel_new (child->conn_mgr, 0, "urn:aspl.es:beep:profiles:reg-test:profile-10b-internal",
				      NULL, NULL, NULL, NULL, NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (20): expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		return axl_false;
	}

	printf ("Test 10-b: channel inside master <-> child conn mgr channel num=%d (%p) created..\n", 
		vortex_channel_get_number (channel), channel);	

	/* send frame received */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	/* send message */
	if (! vortex_channel_send_msg (channel, "helo", 4, NULL)) {
		printf ("ERROR (19): expected to send check conn mgr but found a failure..\n");
		return axl_false;
	} /* end if */

	/* wait reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (16): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	/* check reply */
	if (! axl_cmp (vortex_frame_get_payload (frame), "hola")) {
		printf ("ERROR (21): expected to find 'hola' as content but found: %s\n", 
			(const char *) vortex_frame_get_payload (frame));
		return axl_false;
	}

	printf ("Test 10-b: received expeted content '%s' from internal master<->child conn mgr..ok\n",
		(const char *) vortex_frame_get_payload (frame));
	
	/* free frame */
	vortex_frame_unref (frame);

	/* close connection */
	vortex_connection_close (conn);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10prev);

	return axl_true;
}

VortexConnection * test_10_c_connect_and_check (VortexCtx * vCtx, TurbulenceCtx * ctx, axl_bool channel_should_fail)
{
	
	VortexConnection * conn;
	VortexChannel    * channel;

	/* create connection */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		exit (-1);
	} /* end if */
	
	printf ("Test 10-c: connection id=%d (%p) created..\n", 
		vortex_connection_get_id (conn), conn);

	/* check to create profile 2 channel: MUST WORK */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel_should_fail) {
		printf ("Test 10-c: channel should fail...checking: %p\n", channel);
		if (channel != NULL) {
			printf ("ERROR (2): test-10-c: expected to find NULL channel reference (creation ok) but found failure..\n");
			exit (-1);
		}
	} else {
		printf ("Test 10-c: channel should NOT fail...checking: %p\n", channel);
		if (channel == NULL) {
			printf ("ERROR (2): test-10-c: expected to NOT find NULL channel reference (creation ok) but found failure..\n");
			exit (-1);
		}

		/* check connection after created it */
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (4): test-10-c: expected to find proper connection after turbulence startup..\n");
			exit (-1);
		} /* end if */

	} /* end if */

	printf ("Test 10-c: channel num=%d (%p) created..\n", 
		vortex_channel_get_number (channel), channel);

	/* now wait a bit 100ms */
	turbulence_sleep (tCtxTest10prev, 100000);

	return conn;
}
						     

axl_bool test_10_c (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn, * conn2, * conn3;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10prev, "test_10c.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10prev)) 
		return axl_false;

	/* install signal handling (handle child processes) */
	turbulence_signal_install (tCtxTest10prev, axl_false, axl_false, test_10_prev_signal_handler);

	/* create connection to local server */
	printf ("Test 10-c: CREATE FIRST CONNECTION(1): creating child process..\n");
	conn = test_10_c_connect_and_check (vCtx, tCtxTest10prev, axl_false);

	/* now create a second child */
	printf ("Test 10-c: CREATING SECOND CONNECTION(2): attempting to create a second child to see if it is allowed..\n");

	/* second reference */
	conn2 = test_10_c_connect_and_check (vCtx, tCtxTest10prev, axl_true);

	printf ("Test 10-c: check childs (first: should be 1)..\n");
	if (turbulence_process_child_count (tCtxTest10prev) != 1) {
		printf ("ERROR (8): expected to find child count 1 but found %d..\n", turbulence_process_child_count (tCtxTest10prev));
		return axl_false;
	} /* end if */

	/* ok, close connection not used */
	vortex_connection_close (conn2);

	printf ("Test 10-c: changing run time settings to accept 3 childs and check profile path limits..\n");
	tCtxTest10prev->global_child_limit = 3;

	printf ("Test 10-c: CREATING SECOND CONNECTION(3): creating a second connection again (it should work..)\n");

	/* second reference */
	conn2 = test_10_c_connect_and_check (vCtx, tCtxTest10prev, axl_false);


	printf ("Test 10-c: check childs (second check, should be 2)..\n");
	if (turbulence_process_child_count (tCtxTest10prev) != 2) {
		printf ("ERROR (8): expected to find child count 1 but found %d..\n", turbulence_process_child_count (tCtxTest10prev));
		return axl_false;
	} /* end if */

	printf ("Test 10-c: ok, now create a third connection that should fail because profile path limits..\n");

	/* second reference */
	conn3 = test_10_c_connect_and_check (vCtx, tCtxTest10prev, axl_true);

	printf ("Test 10-c: check childs..\n");
	if (turbulence_process_child_count (tCtxTest10prev) != 2) {
		printf ("ERROR (8): expected to find child count 2 but found %d..\n", turbulence_process_child_count (tCtxTest10prev));
		return axl_false;
	} /* end if */
	

	/* close connection */
	vortex_connection_close (conn3);
	vortex_connection_close (conn2);
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10prev);

	return axl_true;
}


int test_10_d_filter_conn (VortexConnection *conn, axlPointer user_data)
{
	VortexConnection * temp;
	TurbulenceCtx    * ctx = user_data;

	if (user_data == NULL) {
		printf ("ERROR: failed to filter connection, expected to find user_data pointer defined..\n");
		return axl_true;
	}

	/* get access to the conn mgr API to ensure we don't lock */
	printf ("Test 10-d: called broadcast filter handler (user_data: %p)..\n", user_data);

	/* get connection */
	temp = turbulence_conn_mgr_find_by_id (ctx, vortex_connection_get_id (conn));

	/* check reference */
	if (temp == NULL) {
		printf ("ERROR: expected to find connection reference at filter handler but found NULL\n");
		return axl_true; /* filter to make test fail */
	}
	
	if (conn != temp) {
		printf ("ERROR: expected to find connection reference %p, but found %p\n", conn, temp);
		return axl_true; /* filter to make test fail */
	}

	/* do not filter */
	return axl_false;
}

axl_bool test_10_d (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	
	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_10d.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");

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
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel == NULL) {
		printf ("ERROR (2): expected to find NOT NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	} /* end if */

	/* create async queue and configure frame received handler */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	/* now call to broadcast a message */
	printf ("Test 10-d: sending broadcast message..\n");
	if (! turbulence_conn_mgr_broadcast_msg (tCtx, 
						 "This is a test",
						 14, 
						 "urn:aspl.es:beep:profiles:reg-test:profile-1",
						 test_10_d_filter_conn, 
						 tCtx)) {
		printf ("ERROR (3): failed to broadcast message...\n");
		return axl_false;
	}

	/* get reply */
	printf ("Test 10-d: getting reply message..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (4): expected to find a frame reference defined but NULL value was found...\n");
		return axl_false;
	}

	/* check value */
	if (! axl_cmp (vortex_frame_get_payload (frame), "This is a test")) {
		printf ("ERROR (5): expected to find content 'This is a test' but found: '%s'\n", (const char *) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* release frame */
	vortex_frame_unref (frame);
	vortex_async_queue_unref (queue);

	/* terminate connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_10_e (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	
	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_10e.conf")) 
		return axl_false;

	/* register here all profiles required by tests */
	SIMPLE_URI_REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1");

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create connection to local server */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "dk534jd.fail.aspl.es", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	if (channel != NULL) {
		printf ("ERROR (2): expected to find NULL channel reference (creation failure) but found proper result..\n");
		return axl_false;
	} /* end if */

	/* close the connection and check child process */
	printf ("Test 10-e: closing connection and checking childs..\n");
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_10_a (void) {

	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtxTest10a, "test_10-a.conf"))
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtxTest10a)) 
		return axl_false;

	/* install signal handling */
	turbulence_signal_install (tCtxTest10a, axl_false, axl_false, test_10_a_signal_handler);

	/* create connection to local server */
	printf ("Test 10-a: creating connection..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST WORK */
	printf ("Test 10-a: opening channel...\n");
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-1-failed");
	if (channel == NULL) {
		printf ("ERROR (2): test-10-c: expected to NOT find NULL channel reference (creation ok) but found failure..\n");
		return axl_false;
	}

	/* check connection after created it */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (4): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* ask for remote pid and compare it to the current value */
	printf ("Test 10-a: send message to break child...\n");
	if (! vortex_channel_send_msg (channel, "wrong access", 12, NULL)) {
		printf ("ERROR (6): expected to find remote pid request message sent successfully but found an error..\n");
		return axl_false;
	} /* end if */

	printf ("Test 10-a: now wait to create another connection (300ms)..\n");
	turbulence_sleep (tCtxTest10a, 1000000);

	/* close connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	printf ("Test 10-a: creating (AGAIN) the connection..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-10.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* check to create profile 2 channel: MUST WORK */
	printf ("Test 10-a: opening channel...\n");
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

	/* close the connection and check child process */
	printf ("Test 10: closing connection and checking childs..\n");
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtxTest10a);

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-11.server", VORTEX_OPTS_END),
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
	int                iterator;

	/* create queue  and link it to turbulence ctx */
	queue = vortex_async_queue_new ();
	turbulence_ctx_set_data_full (tCtx, axl_strdup_printf ("%p", queue), queue,
				      axl_free, (axlDestroyFunc) vortex_async_queue_unref);

	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.server", VORTEX_OPTS_END),
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
		printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	}
	
	/* send a message and check result */
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
		iterator = 0;
		while (iterator < axl_list_length (connList)) {
			/* get connection list */
			conn = axl_list_get_nth (connList, iterator);
			
			printf ("  Id=%d Role=%d Socket=%d\n", vortex_connection_get_id (conn), vortex_connection_get_role (conn),
				vortex_connection_get_socket (conn));

			/* next position */
			iterator++;
		}
		return axl_false;
	} /* end if */
	axl_list_free (connList);

	/* close connection */
	vortex_connection_close (conn);
	printf ("Test 12: connection closed..waiting conn mgr to release connection\n");

	/* now wait a bit 1seg */
	iterator = 0;
	while (iterator < 500) {
		
		/* wait a bit */
		turbulence_sleep (tCtx, 10000);

		/* check child count */
		connList = turbulence_conn_mgr_conn_list (tCtx, -1, NULL);
		if (axl_list_length (connList) == connections_after_close) {
			axl_list_free (connList);
			break;
		} /* end if */
		axl_list_free (connList);

		/* next */
		iterator++;
	}

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.another-server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (10): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */	

	/* enable SASL auth for current connection */
	printf ("Test 12: checking aspl user is not authenticated for other serverName (expected failure..)\n");
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
	printf ("Test 12: checking aspl2 user IS authenticated for other serverName (not expected failure..)\n");
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl2", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);

	if (status != VortexOk) {
		printf ("ERROR (13): expected to not find auth failure for aspl2 user under test-12.another-server, but error found was: (%d) %s..\n", status, status_message);
		printf ("            showing connection errors: \n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (14): expected to find proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}

	vortex_connection_close (conn);
	printf ("Test 12: connection closed (2)..\n");


	/*** now connect using test-12.another-server to use another database **/
	if (test_local_sasl) {
		printf ("Test 12: testing test-12.third-server, user defined SASL database..\n");
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-12.third-server", VORTEX_OPTS_END),
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
			printf ("Test 12: checking process count list %d (current pid: %d)..\n", turbulence_process_child_count (tCtx), getpid ());
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
	turbulence_signal_install (tCtx, axl_false, axl_false, test_12_signal_received);

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

	/* remove signal handler */
	turbulence_signal_install (tCtx, axl_false, axl_false, NULL);

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("sasl"), 
						   vortex_support_build_filename ("test_12_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* call implement common test_12 */
	return test_12_common (vCtx, tCtx, 3, 1, axl_false);
}

axl_bool test_12b (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;

	printf ("Test 12-b: PLEASE, ensure you have a database created with data found in: test_12b.sql\n");

	/* FIRST PART: init vortex and turbulence (same test as
	   test_12a but change databases to be managed by a mysql
	   server) */
	if (! test_common_init (&vCtx, &tCtx, "test_12a.conf")) 
		return axl_false;

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("sasl"), 
						   vortex_support_build_filename ("test_12b_module", NULL));

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.server", VORTEX_OPTS_END),
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
		show_conn_errors (conn);
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test");
	if (channel == NULL) {
		printf ("ERROR (4): expected to proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check", 12, NULL)) {
		printf ("ERROR (5): expected to send content but found error..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (6): expected to find reply for get pid request...\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 1")) {
		printf ("ERROR (7): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_close (conn);

	/* --- TEST: test wrong initialization --- */
	printf ("Test 13: checking wrong initialization..\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.wrong.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (8): expected to find proper connection after turbulence startup..\n");
		show_conn_errors (conn);
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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.another-server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (9): expected to find proper connection after turbulence startup..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl2", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
	
	if (status != VortexOk) {
		printf ("ERROR (10): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	printf ("Test 13: authentication under domain test-13.another-server COMPLETE\n");

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (11): expected to proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test-2");
	if (channel == NULL) {
		printf ("ERROR (12): expected to proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}
	/* unref the queue */
	vortex_async_queue_unref (queue);
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check-2", 14, NULL)) {
		printf ("ERROR (13): expected to send content but found error..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (14): expected to find reply for get pid request...\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 2")) {
		printf ("ERROR (15): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		show_conn_errors (conn);
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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.third-server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (16): expected to find proper connection after turbulence startup..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* enable SASL auth for current connection */
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl-3", NULL);
	vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
	vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);

	
	if (status != VortexOk) {
		printf ("ERROR (17): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	printf ("Test 13: authentication under domain test-13.third-server COMPLETE\n");

	/* now create a channel (just to check channels provided by other modules) */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-11");
	if (channel == NULL) {
		printf ("ERROR (18): expected to proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}

	/* now create a channel registered by python code */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:python-test-3");
	if (channel == NULL) {
		printf ("ERROR (19): expected to proper channel creation but a failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	}
	/* unref the queue */
	vortex_async_queue_unref (queue);
	
	/* send a message and check result */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	if (! vortex_channel_send_msg (channel, "python-check-3", 14, NULL)) {
		printf ("ERROR (20): expected to send content but found error..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (21): expected to find reply for get pid request...\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, this is python app 3")) {
		printf ("ERROR (22): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		show_conn_errors (conn);
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

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* flag mod python to be not unloaded from memory because it
	 * breaks if it is unloaded and reloaded again on next python
	 * tests */
	turbulence_module_skip_unmap (tCtx, "mod_python");

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

	/* flag mod python to be not unloaded from memory because it
	 * breaks if it is unloaded and reloaded again on next python
	 * tests */
	turbulence_module_skip_unmap (tCtx, "mod_python");

	/* call to test common python functions */
	if (! test_13_common (vCtx, tCtx, axl_true))
		return axl_false;


	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_13_b (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_13b.conf")) 
		return axl_false;

	/* configure test path to locate appropriate sasl.conf files */
	vortex_support_add_domain_search_path_ref (vCtx, axl_strdup ("python"), 
						   vortex_support_build_filename ("test_13_module", NULL));

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* flag mod python to be not unloaded from memory because it
	 * breaks if it is unloaded and reloaded again on next python
	 * tests */
	turbulence_module_skip_unmap (tCtx, "mod_python");

	/* now open connection to localhost */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-13.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */

	/* unregister from turbulence to avoid echo effect. Because
	 * all connections created under the same process are
	 * registered into turbulence, we want to unregister to
	 * simulate independent connections */
	turbulence_conn_mgr_unregister (tCtx, conn);
	
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

	/* get the frame */
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

	/* now send message that should be not broadcasted */
	printf ("Test 13-b: BROADCAST 1: sending initial test..\n");
	if (! vortex_channel_send_msg (channel, "broadcast 1", 11, NULL)) {
		printf ("ERROR (8): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	/* get the frame */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (9): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, broadcast 1")) {
		printf ("ERROR (10): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* now send message that should be not broadcasted */
	printf ("Test 13-b: BROADCAST 2: sending initial test..\n");
	if (! vortex_channel_send_msg (channel, "broadcast 2", 11, NULL)) {
		printf ("ERROR (11): expected to send content but found error..\n");
		return axl_false;
	} /* end if */

	/* get the frame */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (12): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "hey, broadcast 2")) {
		printf ("ERROR (13): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* get the frame (from the broadcast) */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (14): expected to find reply for get pid request...\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "This should reach")) {
		printf ("ERROR (15): expected to find 'profile path notified' but found '%s'",
			(const char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* clear frame */
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_shutdown (conn);
	vortex_connection_close (conn);

	/* finish queue */
	vortex_async_queue_unref (queue);

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-14.server", VORTEX_OPTS_END),
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

	/* check servername again */
	if (! axl_cmp (vortex_connection_get_server_name (conn), "test-14.server")) {
		printf ("ERROR (3): expected to find test-14.server serverName but found: %s..\n", vortex_connection_get_server_name (conn));
		return axl_false;
	} /* end if */

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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server", VORTEX_OPTS_END),
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
					    CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server", VORTEX_OPTS_END),
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
					    CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-15.server", VORTEX_OPTS_END),
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
	int               ppath_id;
	int               has_tls;
	int               fix_server_name;
	const char      * remote_host = "localhost";
	const char      * remote_port = "1233";
	const char      * remote_host_ip = "127.0.0.1";

	/* build connection status */
	conn_status = turbulence_process_connection_status_string (axl_true,
								   3, 
								   "urn:aspl.es:beep:profiles:reg-test:profile-15",
								   NULL,
								   EncodingNone,
								   "test-15.server",
								   17, 42301, 1234, 37, 1, 0, remote_host, remote_port, remote_host_ip, 0);
	turbulence_process_connection_recover_status (conn_status + 1, /* skip initial n used to signal the command */
						      &handle_start_reply,
						      &channel_num,
						      &profile,
						      &profile_content,
						      &encoding,
						      &serverName,
						      &msg_no,
						      &seq_no, 
						      &seq_no_expected, 
						      &ppath_id,
						      &fix_server_name,
						      &remote_host, &remote_port, &remote_host_ip,
						      &has_tls);
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

	if (ppath_id != 37) {
		printf ("ERROR (10): unexpected profile path id value (%d != 37)\n", ppath_id);
		return axl_false;
	}

	axl_free (conn_status);
	return axl_true;
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

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* connect to local server */
	iterator = 0;
	while (iterator < connections) {
		/* create a connection */
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-16.server", VORTEX_OPTS_END),
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
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-16.server.child", VORTEX_OPTS_END),
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
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-17.server", VORTEX_OPTS_END),
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
	if (! test_common_init (&vCtx, &tCtx, "test_17.conf")) {
		printf ("ERROR: failed to init Turbulence Engine or Vortex Engine..\n");
		return axl_false;
	}

	/* run configuration */
	/* turbulence_log_enable (tCtx, axl_true);
	   turbulence_color_log_enable (tCtx, axl_true);   */
	if (! turbulence_run_config (tCtx)) {
		printf ("ERROR: failed to start running Turbulence Engine..\n");
		return axl_false;
	}

	/* turbulence_log_enable (tCtx, axl_true);
	  turbulence_color_log_enable (tCtx, axl_true);  */
	/* vortex_log_enable (vCtx, axl_true);
	vortex_log2_enable (vCtx, axl_true);
	vortex_color_log_enable (vCtx, axl_true); */
	
	printf ("Test 17: creating %d connections with an opened channel..\n", connections);
	iterator = 0;
	while (iterator < connections) {

		/* open a connection */
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-17.server", VORTEX_OPTS_END),
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
			printf ("ERROR (2): expected to find proper channel creation but a failure was found during iterator=%d..\n",
				iterator);
			printf ("           showing errors:\n");
			show_conn_errors (conn);
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
 * @brief Test the case when a child process is created for a
 * connection (due to profile path configuration) but at the end the
 * channel requested is not accepted.
 */
axl_bool test_18 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_18.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* configure client conn created handler */
	vortex_ctx_set_client_conn_created (vCtx, test_01_conn_created, tCtx);

	/* now connect to local host and open a channel */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	printf ("**\n** Test 18: created conn-id=%d (%p)...\n**\n", vortex_connection_get_id (conn), conn);

	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-18");
	if (channel != NULL) {
		printf ("ERROR (2): expected to NOT find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* ok, now close the connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);
	
	return axl_true;
}

/** 
 * @brief Test the case when a child process is created for a
 * connection (due to profile path configuration) but at the end the
 * channel requested is not accepted.
 */
axl_bool test_19 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_19.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* now connect to local host and open a channel */
	conn = vortex_connection_new (vCtx, "127.0.0.1", "44010", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-17");
	if (channel != NULL) {
		printf ("ERROR (2): expected to NOT find proper channel creation but a failure was found..\n");
		return axl_false;
	}

	/* ok, now close the connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);
	
	return axl_true;
}

axl_bool test_20_check_child_profile_path (TurbulenceCtx * tCtx, VortexCtx * vCtx, VortexChannel * channel)
{
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	printf ("Test 20: sending get-profile-path-name command..\n");
	vortex_channel_send_msg (channel, "get-profile-path-name", 21, NULL);
	frame = vortex_channel_get_reply (channel, queue);

	/* check frame content */
	if (! axl_cmp ((const char *) vortex_frame_get_payload (frame), "test 20 profile path")) {
		printf ("ERROR (5): Expected to find profile path name '%s' but found '%s'\n",
			"test 20 profile path", (const char *) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */
	
	/* now, release content */
	vortex_frame_unref (frame);
	vortex_async_queue_unref (queue);

	return axl_true;
}

axl_bool test_20 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn2;
	VortexConnection * conn;
	

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_20.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* FIRST: open a connection to force the second connection to be passed to an already created child. */ 
	/* now connect to local host and open a channel */
	conn2 = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					    CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-20.server", VORTEX_OPTS_END),
					    NULL, NULL);
	if (! vortex_connection_is_ok (conn2, axl_false)) {
		printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn2), vortex_connection_get_message (conn2));
		return axl_false;
	} /* end if */

	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn2, "urn:aspl.es:beep:profiles:reg-test:profile-20:1");
	if (channel == NULL) {
		printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* SECOND: no create the second connection (reusing child) */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-20.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (3): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-20:1");
	if (channel == NULL) {
		printf ("ERROR (4): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check we have a profile path defined */
	if (! test_20_check_child_profile_path (tCtx, vCtx, channel))
		return axl_false;

	/* now check to create second profile channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-20:3");
	if (channel != NULL) {
		printf ("ERROR (5): expected to NOT find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */


	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-20:2");
	if (channel == NULL) {
		printf ("ERROR (6): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */


	/* ok, now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-20:3");
	if (channel == NULL) {
		printf ("ERROR (7): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* ok, now close the connection */
	vortex_connection_close (conn);
	vortex_connection_close (conn2);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

axl_bool test_21 (void) {

	VortexAsyncQueue * queue = NULL;
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexConnection * conn[10];
	int                iterator;
	VortexFrame      * frame;
	int                conn_id;
	int                previous_conn_id  = -1;


	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_20.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create connections and channels */
	iterator = 0;
	while (iterator < 10) {
		conn[iterator] = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-20.server", VORTEX_OPTS_END),
						   NULL, NULL);
		if (! vortex_connection_is_ok (conn[iterator], axl_false)) {
			printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
				vortex_connection_get_status (conn[iterator]), vortex_connection_get_message (conn[iterator]));
			return axl_false;
		} /* end if */
		
		/* ok, now create a channel */
		channel = SIMPLE_CHANNEL_CREATE_WITH_CONN (conn[iterator], "urn:aspl.es:beep:profiles:reg-test:profile-20:1");
		if (channel == NULL) {
			printf ("ERROR (2): expected to find proper channel creation but a failure was found..\n");
			return axl_false;
		} /* end if */

		/* init queue */
		if (queue == NULL)
			queue = vortex_async_queue_new ();

		/* send request to get connection id */
		vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
		vortex_channel_send_msg (channel, "send-msg", 8, NULL);
		
		/* get reply */
		frame = vortex_channel_get_reply (channel, queue);
		conn_id = atoi ((const char *) vortex_frame_get_payload (frame));
		vortex_frame_unref (frame);
		
		printf ("Test 21: connection ID from remote host: %d\n", conn_id);

		if (previous_conn_id >= conn_id) {
			printf ("ERROR: expected to find different connection ID (and bigger) but found next value: previous-conn-id:(%d) >= conn-id:(%d)\n",
				previous_conn_id, conn_id);
			return axl_false;
		} /* end if */

		/* update connection id */
		previous_conn_id = conn_id;

		iterator++;
	} /* end if */

	iterator = 0;
	while (iterator < 10) {
		vortex_connection_shutdown (conn[iterator]);
		vortex_connection_close (conn[iterator]);
		iterator++;
	}

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);
	
	return axl_true;
}

axl_bool test_22_operations (TurbulenceCtx * ctx, VortexCtx * vCtx, const char * serverName, VortexAsyncQueue * queue, axl_bool do_sasl_before, axl_bool use_ws) {
	VortexConnection * conn = NULL;
	VortexChannel    * channel;
	VortexFrame      * frame;
	VortexStatus       status;
	char             * status_message = NULL;
	int                iterator;

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	VortexWebsocketSetup * wss_setup;

	if (use_ws) {
		/* create Websocket connection */
		msg ("Test --: Creating websocket connection (Host: %s): 127.0.0.1:1602", serverName);
		iterator = 0;
		while (iterator < 2) {

			/* connect and enable TLS through websocket protocol */
			wss_setup = vortex_websocket_setup_new (vCtx);
			/* setup we want TLS enabled */
			vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_TLS, INT_TO_PTR (axl_true));
			/* setup Host: header to indicate the profile path or application we want */
			vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_HOST, (axlPointer) serverName);

			/* disable SSL verification */ 
			vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_CERT_VERIFY, INT_TO_PTR (axl_false));

/*			if (iterator > 0) { */
				/* enable log at nopoll */
				/* vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_DEBUG, INT_TO_PTR (axl_true)); */
				
				/* enable log */
				/* vortex_log_enable (vCtx, axl_true);
				   vortex_color_log_enable (vCtx, axl_true);  */
			/* } */ /* end if */

			/* create new connection */
			conn = vortex_websocket_connection_new ("127.0.0.1", "1602", wss_setup, NULL, NULL);
			if (vortex_connection_is_ok (conn, axl_false))
				break;
			/* release connection */
			vortex_connection_close (conn);

			iterator++;
			turbulence_sleep (ctx, 10000); 
			printf ("Test --: ...failed to create Web-Socket connection, trying again..\n");
		} /* end while */
		
		/* check connection again */
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
				vortex_connection_get_status (conn), vortex_connection_get_message (conn));
			return axl_false;
		} /* end if */
	} else {
#endif
		/* connect and enable TLS */
		conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
						   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, serverName, VORTEX_OPTS_END),
						   NULL, NULL);
		if (! vortex_connection_is_ok (conn, axl_false)) {
			printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
				vortex_connection_get_status (conn), vortex_connection_get_message (conn));
			return axl_false;
		} /* end if */
		
		/* enable TLS */
		conn = vortex_tls_start_negotiation_sync (conn, serverName, &status, &status_message);
		if (status != VortexOk) {
			printf ("ERROR (2): expected to find proper TLS activation but found a failure: %s\n",
				status_message);
			return axl_false;
		} /* end if */

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	}
#endif

	printf ("Test --: TLS activation finished: serverName %s..\n", serverName);
	if (! vortex_connection_is_tlsficated (conn)) {
		printf ("ERROR (2.1): expected to find proper TLS activation..\n");
		return axl_false;
	} /* end if */

	printf ("Test --: Creating channel to test TLS: serverName %s..\n", serverName);

	if (do_sasl_before) {
		/* check that the channel is not available withtout SASL */
		printf ("Test --: creating reg-test:profile-22:1 before activating SASL (expecting failure..)\n");
		channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
		if (channel != NULL) {
			printf ("ERROR (2.2): expected to NOT find proper channel creation but a proper reference was found..\n");
			return axl_false;
		} /* end if */

		/* ok, do sasl */
		/* enable SASL auth for current connection */
		printf ("Test --: activating SASL user auth (aspl:test)\n");
		vortex_sasl_set_propertie (conn,   VORTEX_SASL_AUTH_ID,  "aspl", NULL);
		vortex_sasl_set_propertie (conn,   VORTEX_SASL_PASSWORD, "test", NULL);
		vortex_sasl_start_auth_sync (conn, VORTEX_SASL_PLAIN, &status, &status_message);
		
		if (status != VortexOk) {
			printf ("ERROR (2.3): expected proper auth for aspl user, but error found was: (%d) %s..\n", status, status_message);
			return axl_false;
		} /* end if */
	} /* end if */

	/* now create a channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
	if (channel == NULL) {
		printf ("ERROR (3): expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* send request to get connection id */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	vortex_channel_send_msg (channel, "send-msg", 8, NULL);
	
	/* get reply */
	frame = vortex_channel_get_reply (channel, queue);
	
	if (! axl_cmp (vortex_frame_get_payload (frame), "send-msg")) {
		printf ("ERROR (4): expected to find 'send-msg' as content but found: %s\n",
			(char *) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	vortex_frame_unref (frame);

	/* check the servername that has the connection on remote side */
	vortex_channel_send_msg (channel, "getServerName", 13, NULL);
	
	/* get reply */
	frame = vortex_channel_get_reply (channel, queue);
	
	if (! axl_cmp (vortex_frame_get_payload (frame), serverName)) {
		printf ("ERROR (5): expected to find serverName %s but found: %s\n",
			serverName, (char *) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */
	printf ("Test 22: found remote serverName: %s..\n", (const char *) vortex_frame_get_payload (frame));

	vortex_frame_unref (frame);

	/* check local servername */
	if (! axl_cmp (vortex_connection_get_server_name (conn), serverName)) {
		printf ("ERROR (6): expected to find local serverName configured (%s) but found %s\n",
			serverName, vortex_connection_get_server_name (conn));
		return axl_false;
	} /* end if */
	printf ("Test 22: found local serverName: %s..\n", vortex_connection_get_server_name (conn));
	
	/* close connection */
	vortex_connection_close (conn);

	/* wait a bit to ensure all handlers are run */
	turbulence_sleep (ctx, 10000);

	return axl_true;
}

axl_bool test_22_unfinished (VortexCtx * vCtx, const char * serverName, VortexAsyncQueue * queue) {
	VortexConnection * conn;
	VortexChannel    * channel;

	/* connect and enable TLS */
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010",
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, serverName, VORTEX_OPTS_END),
					   NULL, NULL);

	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	/* now open a channel */
	printf ("Test 22: creating TLS channel to simulate an unfinished TLS handshake..\n");
	channel = vortex_channel_new_full (conn, /* the connection */
					   0,          /* the channel vortex chose */
					   serverName, /* the serverName value (no matter if it is NULL) */
					   /* the TLS profile identifier */
					   VORTEX_TLS_PROFILE_URI,
					   /* content encoding */
					   EncodingNone,
					   /* initial content or piggyback and its size */
					   "<ready />", 9,
					   /* close channel notification: we don't set it. */
					   NULL, NULL,
					   /* frame received notification: we don't set it. */
					   NULL, NULL,
					   /* on channel crated notification: we don't set it.
					    * It is not needed, we are working on a separated
					    * thread. */
					   NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (2): expected to create TLS channel but failure was found..\n");
		show_conn_errors (conn);
		return axl_false;
	} /* end if */

	/* now try to open unauthorized channel */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
	if (channel != NULL) {
		printf ("ERROR (3): SECURITY ERROR: expected to find proper channel creation but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (conn);

	return axl_true;
}

axl_bool test_22 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_22.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;

	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-22.server */
	printf ("Test 22: checking TLS with: test-22.server..\n");
	if (! test_22_operations (tCtx, vCtx, "test-22.server", queue, axl_false, axl_false)) {
		printf ("ERROR: failed first part\n");
		return axl_false;   
	}

	/* call to test against test-22.server.nochild */
	printf ("Test 22: checking TLS with: test-22.server.nochild..\n");
	if (! test_22_operations (tCtx, vCtx, "test-22.server.nochild", queue, axl_false, axl_false)) {
		printf ("ERROR: failed second part\n");
		return axl_false;   
	}

	/* call to test against test-22.server.sasl ( */
	printf ("Test 22: checking TLS with: test-22.server.sasl..\n");
	if (! test_22_operations (tCtx, vCtx, "test-22.server.sasl", queue, axl_true, axl_false)) {
		printf ("ERROR: failed third part\n");
		return axl_false;   
	}

	/* call to test against test-22.server.sasl.nochild */
	printf ("Test 22: checking TLS with: test-22.server.sasl.nochild..\n");
	if (! test_22_operations (tCtx, vCtx, "test-22.server.sasl.nochild", queue, axl_true, axl_false)) {
		printf ("ERROR: failed fourth part\n");
		return axl_false;   
	}

	/* check unfinished tls */
	if (! test_22_unfinished (vCtx, "test-22.server", queue)) {
		printf ("ERROR: failed unfinished part\n");
		return axl_false;   
	}

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);
	
	return axl_true;
}

axl_bool test_22_a (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_22.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-22.server.sasl.nochild */
	printf ("Test 22: checking TLS with: test-22.server.sasl.nochild..\n");
	if (! test_22_operations (tCtx, vCtx, "test-22.server.sasl.nochild", queue, axl_true, axl_false)) 
		return axl_false;

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);
	
	return axl_true;
}

axl_bool test_23 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_23.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-22.server */
	if (! test_22_operations (tCtx, vCtx, "test-22.server", queue, axl_false, axl_false)) 
		return axl_false;  

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);
	
	return axl_true;
}

axl_bool test_24 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexChannel    * channel;
	VortexChannel    * channel2;
	VortexConnection * conn;

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_22.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) {
		printf ("ERROR: failed to start configuration..\n");
		return axl_false;
	}

	/* create connection to local server */
	printf ("**\n** Test 24: attempting to create connection to the SERVER: test-22.server\n**\n");
	conn = vortex_connection_new_full (vCtx, "127.0.0.1", "44010", 
					   CONN_OPTS(VORTEX_SERVERNAME_FEATURE, "test-22.server", VORTEX_OPTS_END),
					   NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected to find proper connection after turbulence startup..\n");
		return axl_false;
	} /* end if */
	
	/* create TLS channel to simulate unfinished TLS handshake */
	printf ("**\n** Test 24: connection ok, now attempt to create TLS channel (http://iana.org/beep/TLS\n**\n");
	channel = vortex_channel_new_full (conn, 0, "test-22.server", 
					   "http://iana.org/beep/TLS", 
					   EncodingNone, "<ready />", 9,
					   NULL, NULL, 
					   NULL, NULL, 
					   NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (1.1): expected to find proper channel creation, but found a failure\n");
		return axl_false;
	} /* end if */

	/* now try to create a protected (unallowed channel inside) */
	channel2 = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
	if (channel2 != NULL) {
		printf ("ERROR (1.2): SECURTY BREACH: expected to not be able to open TLS protected channel, but the channel was created\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	return axl_true;
}

#if defined(ENABLE_WEBSOCKET_SUPPORT)
#define h_addr h_addr_list[0]
VORTEX_SOCKET _test_create_socket (const char * host, const char * port)
{
	struct hostent     * hostent;
	struct sockaddr_in   saddr;
	VORTEX_SOCKET        session;

	/* resolve hosting name */
	hostent = gethostbyname (host);
	if (hostent == NULL) {
		printf ("unable to resolve host name %s", host);
		exit (-1);
	} /* end if */

	/* create the socket and check if it */
	session      = socket (AF_INET, SOCK_STREAM, 0);
	if (session == VORTEX_INVALID_SOCKET) {
		printf ("unable to create socket");
		exit (-1);
	} /* end if */

	/* prepare socket configuration to operate using TCP/IP
	 * socket */
        memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = ((struct in_addr *)(hostent->h_addr))->s_addr;
        saddr.sin_family    = AF_INET;
        saddr.sin_port      = htons((uint16_t) strtod (port, NULL));

	/* do a tcp connect */
        if (connect (session, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(errno != VORTEX_EINPROGRESS && errno != VORTEX_EWOULDBLOCK) { 
			shutdown (session, SHUT_RDWR);
			nopoll_close_socket (session);
			/* printf ("unable to connect to remote host %s:%s errno=%d\n", host, port, errno); */
			return -1;
		} /* end if */
	} /* end if */

	/* return socket created */
	return session;
}

axl_bool test_websocket_listener_disabled (const char * server, const char * port)
{
	VORTEX_SOCKET     _socket;
	int               tries   = 0;
	char            * cmd;

	while (axl_true) {
		/* create socket connection */
		_socket = _test_create_socket (server, port);
		if (_socket <= 0)
			break;

		if (_socket > 0 && tries > 100) {

			printf ("ERROR: expected to not to find port %s:%s to be running, but it was found a complete connect socket=%d\n",
				server, port, _socket);
#if defined(AXL_OS_UNIX)
			printf ("ERROR: current listener connections: \n");
			cmd = axl_strdup_printf ("netstat --inet -a -n -p | grep :%s", port);
			if (! system (cmd)) {
			  printf ("ERROR: netstat command failed..\n");
			}
			axl_free (cmd);
#endif
			return axl_false;
		} /* end if */

		/* close socket and release context */
		vortex_close_socket (_socket);
		tries++;
		turbulence_sleep (NULL, 100000);
	}  /* end while */

	return axl_true;
}

axl_bool test_25 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	printf ("Test 25: running websocket test..\n");

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_25.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-22.server.nochild */
	printf ("Test 25: checking TLS with: test-25.server.nochild..\n");
	if (! test_22_operations (tCtx, vCtx, "test-25.server.nochild", queue, axl_false, axl_true))  
		return axl_false;   

	printf ("Test 25: complete OK\n");

	/* call to test against test-22.server.sasl.nochild */
	printf ("Test 25: checking TLS with: test-25.server.sasl.nochild..\n");
	if (! test_22_operations (tCtx, vCtx, "test-25.server.sasl.nochild", queue, axl_true, axl_true)) 
		return axl_false;   

	printf ("Test 25: complete OK\n");

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;
	
	return axl_true;
}

axl_bool test_26 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;


	printf ("Test 26: running websocket test..\n");

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_25.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-26.server */
	printf ("Test 26: checking TLS with: test-25.server..\n");
	if (! test_22_operations (tCtx, vCtx, "test-25.server", queue, axl_false, axl_true))  
		return axl_false;   

	printf ("Test 26: complete OK\n"); 
	/* turbulence_log_enable (tCtx, axl_true);
	turbulence_color_log_enable (tCtx, axl_true);
	vortex_log_enable (vCtx, axl_true);
	vortex_color_log_enable (vCtx, axl_true); */

	/* call to test against test-22.server.sasl */
	printf ("Test 26: checking TLS with: test-25.server.sasl..\n");
	/* turbulence_log_enable (tCtx, axl_true);
	   turbulence_color_log_enable (tCtx, axl_true); */
	if (! test_22_operations (tCtx, vCtx, "test-25.server.sasl", queue, axl_true, axl_true)) 
		return axl_false;   

	printf ("Test 26: complete OK\n");

	/* finish turbulence */
	printf ("Test 26: finishing turbulence (%p) and vortex context (%p)..\n",
		vCtx, tCtx);
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;
	
	return axl_true;
}

axl_bool test_27 (void) {

	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;

	printf ("Test 27: running websocket port sharing detected on child..\n");

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_27.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* call to test against test-26.server */
	printf ("Test 27: checking TLS with: test-25.server..\n");
	/* turbulence_log_enable (tCtx, axl_true);
	   turbulence_color_log_enable (tCtx, axl_true); */

	/* vortex_log_enable (vCtx, axl_true);
	   vortex_color_log_enable (vCtx, axl_true);   */
	if (! test_22_operations (tCtx, vCtx, "test-25.server", queue, axl_false, axl_true))  
		return axl_false;   

	printf ("Test 27: complete OK\n");

	/*	printf ("Test 26: checking TLS with: test-25.server..\n");
	if (! test_22_operations (tCtx, vCtx, "test-25.server", queue, axl_false, axl_true))  
		return axl_false;   

		printf ("Test 26: complete OK\n"); */

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;
	
	return axl_true;
}

axl_bool test_28 (void) {
	TurbulenceCtx    * tCtx;
	VortexCtx        * vCtx;
	VortexAsyncQueue * queue;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexFrame      * frame;
	char             * local_check;

	/* websocket setup */
	VortexWebsocketSetup * wss_setup;

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;

	printf ("Test 28: checking proxy on parent retain original host and port descriptions..\n");

	/* FIRST PART: init vortex and turbulence */
	if (! test_common_init (&vCtx, &tCtx, "test_27.conf")) 
		return axl_false;

	/* run configuration */
	if (! turbulence_run_config (tCtx)) 
		return axl_false;
	
	/* create queue, common to all tests */
	queue = vortex_async_queue_new ();

	/* connect and enable TLS through websocket protocol */
	wss_setup = vortex_websocket_setup_new (vCtx);
	/* setup we want TLS enabled */
	vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_TLS, INT_TO_PTR (axl_true));
	/* vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_DEBUG, INT_TO_PTR (axl_true)); */

	/* create connection */
	conn = vortex_websocket_connection_new ("localhost", "1602", wss_setup, NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: failed to create connection...\n");
		return axl_false;
	} /* end if */

	printf ("Test 28: ok, session created (with local port %s:%s), now create the channel..\n",
		vortex_connection_get_local_addr (conn), vortex_connection_get_local_port (conn));

	/* now create channel to ask the server what port and source
	 * address is perceived */
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
	if (channel == NULL) {
		printf ("ERROR (2.2): expected to find proper channel creation but a proper reference was found..\n");
		return axl_false;
	} /* end if */

	/* send request to get connection id */
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);
	vortex_channel_send_msg (channel, "getRemoteAddress", 16, NULL);

	/* get reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (2.3): failed to get remote address reply..\n");
		return axl_false;
	}

	printf ("Test 28: perceived remote address is: %s\n", (const char *) vortex_frame_get_payload (frame));

	local_check = axl_strdup_printf ("%s:%s", vortex_connection_get_local_addr (conn), vortex_connection_get_local_port (conn));

	printf ("Test 28: checking '%s' == '%s'\n", local_check, (const char *) vortex_frame_get_payload (frame));
	
	if (! axl_cmp (local_check, (const char *) vortex_frame_get_payload (frame))) {
		printf ("ERROR: (2.4): expected a different recognized remote address but found something different: %s\n", local_check);
		return axl_false;
	} /* end if */

	vortex_frame_unref (frame);
	axl_free (local_check);

	/* close connection */
	vortex_connection_close (conn);

	/* finish turbulence */
	test_common_exit (vCtx, tCtx);

	/* finish queue */
	vortex_async_queue_unref (queue);

	/* test local connect to fail */
	if (! test_websocket_listener_disabled ("localhost", "1602"))
		return axl_false;
	
	return axl_true;
}

#endif

typedef axl_bool (* TurbulenceTestHandler) (void);

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
int run_test (TurbulenceTestHandler function, const char * message) {

	if (function ()) {
		printf ("%s [   OK   ]\n", message);
	} else {
		printf ("%s [ FAILED ]\n", message);
		exit (-1);
	}
	return 0;
}

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

#define CHECK_TEST(name) if (run_test_name == NULL || axl_cmp (run_test_name, name))

/** 
 * @brief General regression test to check all features inside
 * turbulence.
 */
int main (int argc, char ** argv)
{
	axl_bool enable_python_tests = axl_false;
	char * run_test_name = NULL;
	axl_bool enable_10a = axl_true;
	axl_bool only_python = axl_false;

	printf ("** test_01: Turbulence BEEP application server regression test\n");
	printf ("** Copyright (C) 2008 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: turbulence: %s \n",
		VERSION);
	printf ("**                   vortex:     %s \n",
		VORTEX_VERSION);
	printf ("**                   axl:        %s\n**\n",
		AXL_VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--python-tests] [--only-python-tests] [--run-test=NAME] [--no-10a] [--child-cmd-prefix]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     PARENT: \n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("**     CHILDREN: \n");
	printf ("**     >> ./test_01 --child-cmd-prefix='libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no' [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_01, test_01, test_01a, test_0b, test_02, test_03, test_03a, test_04, test_05, test_05a, test_06, test_06a\n");
	printf ("**                  test_07, test_08, test_09, test_10prev, test_10, test_10a, test_10b, test_10c, test_10d, test_10e, test_11, test_12,\n");
	printf ("**                  test_12a, test_12b, test_13, test_13a, test_13b, test_14, test_15, test_15a, test_16, test_17, test_18,\n");
	printf ("**                  test_19, test_20, test_21, test_22, test_22a, test_23, test_24, test_25, test_26, test_27, test_28\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <vortex@lists.aspl.es> Vortex/Turbulence Mailing list\n**\n");

	/* set starting process name */
	turbulence_process_set_file_path ("../src/turbulence");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
		if (axl_cmp (argv[argc], "--python-tests")) {
			enable_python_tests = axl_true;
		}
		if (axl_cmp (argv[argc], "--only-python-tests")) {
			only_python = axl_true;
			enable_python_tests = axl_true;
		}
		if (axl_cmp (argv[argc], "--no-10a"))
			 enable_10a = axl_false;
		if (argv[argc] && axl_memcmp (argv[argc], "--child-cmd-prefix", 18)) { 
			printf ("** Setting children cmd prefix: %s\n", argv[argc] + 19);
			turbulence_process_set_child_cmd_prefix (argv[argc] + 19);
		}
		if (argv[argc] && axl_memcmp (argv[argc], "--no-unmap-modules", 18)) { 
			printf ("** Setting no unmap modules: True\n");
			turbulence_module_set_no_unmap_modules (axl_true);
		}
		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	if (only_python)
		goto python_test;

	/* init context to be used on the following tests */
	test_with_context_init ();

	/* run tests */
	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: Turbulence db-list implementation");

	CHECK_TEST("test_01a")
	run_test (test_01a, "Test 01-a: Regular expressions");

	CHECK_TEST("test_0b")
	run_test (test_01b, "Test 01-b: smtp notificaitons");

	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: Turbulence misc functions");

	CHECK_TEST("test_03")
	run_test (test_03, "Test 03: Sasl core backend (used by mod-sasl, tbc-sasl-conf)");

	CHECK_TEST("test_03a")
	run_test (test_03a, "Test 03a: Check crypt backend (common_sasl_check_crypt_password)");

	CHECK_TEST("test_04")
	run_test (test_04, "Test 04: Check module loading support");

	/* terminate context used by previous tests */
	terminate_contexts ();

	CHECK_TEST("test_05")
	run_test (test_05, "Test 05: Check mediator API");

	CHECK_TEST("test_05a")
	run_test (test_05_a, "Test 05-a: Check system user/group id resolving..");
	
	CHECK_TEST("test_06")
	run_test (test_06, "Test 06: Turbulence startup and stop");

	CHECK_TEST("test_06a")
	run_test (test_06_a, "Test 06-a: Check file configuration splitting support");

	CHECK_TEST("test_07")
	run_test (test_07, "Test 07: Turbulence local connection");

	CHECK_TEST("test_08")
	run_test (test_08, "Test 08: Turbulence profile path filtering (basic)");

	CHECK_TEST("test_09")
	run_test (test_09, "Test 09: Turbulence profile path filtering (serverName)");
	
	CHECK_TEST("test_10prev")
	run_test (test_10_prev, "Test 10-prev: Turbulence profile path filtering (simple child processes)");

	CHECK_TEST("test_10")
	run_test (test_10, "Test 10: Turbulence profile path filtering (child processes)");

	if (enable_10a) {
		CHECK_TEST("test_10a")
		run_test (test_10_a, "Test 10-a: Recover from child with failures...");
	}

	CHECK_TEST("test_10b")
	run_test (test_10_b, "Test 10-b: check master-child BEEP link)");

	CHECK_TEST("test_10c")
	run_test (test_10_c, "Test 10-c: check global child limit and profile path limit");

	CHECK_TEST("test_10d")
	run_test (test_10_d, "Test 10-d: unlock turbulence conn-mgr while broadcasing");

	CHECK_TEST("test_10e")
        run_test (test_10_e, "Test 10-e: test child failing at creation time");

	CHECK_TEST("test_11")
	run_test (test_11, "Test 11: Check turbulence profile path selected");

	CHECK_TEST("test_12")
	run_test (test_12, "Test 12: Check mod sasl (profile path selected authentication)"); 

	CHECK_TEST("test_12a")
	run_test (test_12a, "Test 12-a: Check mod sasl (profile path selected authentication, no childs)"); 

	CHECK_TEST("test_12b")
	run_test (test_12b, "Test 12-b: check mod sasl mysql");

	if (enable_python_tests) {
	python_test:
		CHECK_TEST("test_13")
		run_test (test_13, "Test 13: Check mod python");

		CHECK_TEST("test_13a")
		run_test (test_13_a, "Test 13-a: Check mod python (same test, no childs)"); 

		CHECK_TEST("test_13b")
		run_test (test_13_b, "Test 13-b: Check mod python (broadcast and filtering)"); 

		if (only_python) {
			printf ("All python tests OK!\n");
			return 0;
		}
	} /* end if */

	CHECK_TEST("test_14")
	run_test (test_14, "Test 14: Notify different server after profile path selected");

	CHECK_TEST("test_15")
	run_test (test_15, "Test 15: anchillary data for socket passing");

	CHECK_TEST("test_15a")
	run_test (test_15a, "Test 15-a: Child creation with socket passing support");

	CHECK_TEST("test_16")
	run_test (test_16, "Test 16: Connections that were working, must not be available at childs..");

	CHECK_TEST("test_17")
	run_test (test_17, "Test 17: many connections at the same time for a profile path with separate=yes and reuse=yes");

	CHECK_TEST("test_18")
	run_test (test_18, "Test 18: check child process creation that do not accept the connection..");

	CHECK_TEST("test_19")
	run_test (test_19, "Test 19: check child process creation that do not accept the connection (II)..");

	CHECK_TEST("test_20")
	run_test (test_20, "Test 20: check profile path state also applies to childs (with reuse=yes)..");

	CHECK_TEST("test_21")
	run_test (test_21, "Test 21: check connection id on child reuse (with reuse=yes)..");

	CHECK_TEST("test_22")
	run_test (test_22, "Test 22: check TLS module.."); 

	CHECK_TEST("test_22a")
	run_test (test_22_a, "Test 22-a: check TLS module (no child).."); 

	CHECK_TEST("test_23")
	run_test (test_23, "Test 23: check TLS module on child process without serverName.."); 

	CHECK_TEST("test_24")
	run_test (test_24, "Test 24: try to trick TLS profile.."); 

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	CHECK_TEST("test_25")
	run_test (test_25, "Test 25: test connecting BEEP over secure websocket (1602) ..");   

	CHECK_TEST("test_26")
	run_test (test_26, "Test 26: test connecting BEEP over secure websocket (1602) (child support) ..");   

	CHECK_TEST("test_27")
	run_test (test_27, "Test 27: Checking port sharing on child process..");   

	CHECK_TEST("test_28")
	run_test (test_28, "Test 28: Check proxy on parent retains host and port (of the connecting address)..");   
#endif

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */

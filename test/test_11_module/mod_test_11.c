/* mod_test_11 implementation */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

VortexMutex mutex;

/* mod_test_11 init handler */
static int  mod_test_11_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	msg ("Test 11: calling to init module 11..");

	/* init mutex */
	vortex_mutex_create (&mutex);

	return axl_true;
} /* end mod_test_11_init */

/* mod_test_11 close handler */
static void mod_test_11_close (TurbulenceCtx * _ctx) {

	/* terminate mutex */
	vortex_mutex_destroy (&mutex);

	return;
} /* end mod_test_11_close */

/* mod_test_11 reconf handler */
static void mod_test_11_reconf (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_11_reconf */

/* mod_test_11 unload handler */
static void mod_test_11_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_11_unload */

void mod_test_11_frame_received (VortexChannel    * channel, 
				 VortexConnection * connection, 
				 VortexFrame      * frame, 
				 axlPointer         user_data)
{
	/* get the command */
	const char * command = (const char *) vortex_frame_get_payload (frame);
	char       * result;
	axlList    * connList;

	msg ("Received command: '%s'", command);

	if (axl_cmp (command, "content")) {
		if (axl_cmp (vortex_connection_get_server_name (connection), "test-12.third-server")) {
			/* send reply */
			vortex_channel_send_rpy (channel, "profile path notified for test-12.third-server", 46, vortex_frame_get_msgno (frame));
		} else {
			/* send reply */
			vortex_channel_send_rpy (channel, "profile path notified", 21, vortex_frame_get_msgno (frame));
		}
	} else if (axl_cmp (command, "connections count")) {
		/* get the list of connections */
		connList = turbulence_conn_mgr_conn_list (ctx, VortexRoleListener, NULL);
		result   = axl_strdup_printf ("%d", axl_list_length (connList));

		vortex_channel_send_rpy (channel, result, strlen (result), vortex_frame_get_msgno (frame));

		axl_free (result);
		axl_list_free (connList);
	}
	return;
}

/* mod_test_11 ppath-selected handler */
static axl_bool mod_test_11_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {

	/* mutex lock */
	vortex_mutex_lock (&mutex);
	
	/* unlock mutex */
	vortex_mutex_unlock (&mutex);

	msg ("Test 11: received profile path selected %s, connection id %d", 
	     turbulence_ppath_get_name (ppath_selected), vortex_connection_get_id (conn));

	/* register a profile at this point */
	vortex_profiles_register (TBC_VORTEX_CTX (ctx), "urn:aspl.es:beep:profiles:reg-test:profile-11",
				  NULL, NULL, NULL, NULL, mod_test_11_frame_received, NULL);

	/* notification ok */
	return true;
} /* end mod_test_11_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_test_11",
	"Module used to run test_11 and check profile path function",
	mod_test_11_init,
	mod_test_11_close,
	mod_test_11_reconf,
	mod_test_11_unload,
	mod_test_11_ppath_selected
};

END_C_DECLS


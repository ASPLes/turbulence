/* mod_test_10b implementation */
#include <turbulence.h>
#include <turbulence-ctx-private.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* mod_test_10b init handler */
static int  mod_test_10b_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	msg ("Test 10b: calling to init module 10b..");

	

	return axl_true;
} /* end mod_test_10b_init */

/* mod_test_10b close handler */
static void mod_test_10b_close (TurbulenceCtx * _ctx) {


	return;
} /* end mod_test_10b_close */

/* mod_test_10b reconf handler */
static void mod_test_10b_reconf (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_10b_reconf */

/* mod_test_10b unload handler */
static void mod_test_10b_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_10b_unload */

void mod_test_10b_frame_received (VortexChannel    * channel, 
				  VortexConnection * connection, 
				  VortexFrame      * frame, 
				  axlPointer         user_data)
{
	/* get the command */
	const char      * command = (const char *) vortex_frame_get_payload (frame);
	TurbulenceChild * child;
	TurbulenceCtx   * ctx = user_data;

	msg ("Received command: '%s'", command);

	if (axl_cmp (command, "check conn mgr")) {
		/* get child reference */
		child = ctx->child;
		if (! child) {
			vortex_channel_send_err (channel, "Found undefined child structure..", 33, vortex_frame_get_msgno (frame));
			return;
		}

		/* now check connection status */
		if (! vortex_connection_is_ok (child->conn_mgr, axl_false)) {
			vortex_channel_send_err (channel, "Found child conn mgr not working..", 34, vortex_frame_get_msgno (frame));
			return;
		}

		/* now check role */
		if (vortex_connection_get_role (child->conn_mgr) != VortexRoleListener) {
			vortex_channel_send_err (channel, "Found child conn mgr has unexpected role..", 42, vortex_frame_get_msgno (frame));
			return;
		}

		/* reached this point, conn mgr is ok */
		vortex_channel_send_rpy (channel, "connection is ok", 16, vortex_frame_get_msgno (frame));
		return;
	} else if (axl_cmp (command, "helo")) {
		/* reached this point, conn mgr is ok */
		vortex_channel_send_rpy (channel, "hola", 4, vortex_frame_get_msgno (frame));
		return;
	}
	
	return;
}

/* mod_test_10b ppath-selected handler */
static axl_bool mod_test_10b_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {

	msg ("Test 10b: received profile path selected %s, connection id %d", 
	     turbulence_ppath_get_name (ppath_selected), vortex_connection_get_id (conn));

	/* register a profile at this point */
	vortex_profiles_register (TBC_VORTEX_CTX (ctx), "urn:aspl.es:beep:profiles:reg-test:profile-10b",
				  NULL, NULL, NULL, NULL, mod_test_10b_frame_received, _ctx);

	/* register a profile at this point */
	vortex_profiles_register (TBC_VORTEX_CTX (ctx), "urn:aspl.es:beep:profiles:reg-test:profile-10b-internal",
				  NULL, NULL, NULL, NULL, mod_test_10b_frame_received, _ctx);

	/* notification ok */
	return true;
} /* end mod_test_10b_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_test_10b",
	"Module used to run test_10b and check profile path function",
	mod_test_10b_init,
	mod_test_10b_close,
	mod_test_10b_reconf,
	mod_test_10b_unload,
	mod_test_10b_ppath_selected
};

END_C_DECLS


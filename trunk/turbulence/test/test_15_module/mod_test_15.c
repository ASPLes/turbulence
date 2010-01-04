/* mod_test_15 implementation */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

void mod_test_15_frame_received (VortexChannel    * channel, 
				 VortexConnection * connection, 
				 VortexFrame      * frame, 
				 axlPointer         user_data)
{
	char    * result;
	axlList * conn_list;

	/* just echo with the content received */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {

		if (axl_cmp ("connections-count", (const char *) vortex_frame_get_payload (frame))) {
			/* return number of connections handled by connection manager */
			conn_list = turbulence_conn_mgr_conn_list (ctx, -1, NULL);
			result = axl_strdup_printf ("%d", axl_list_length (conn_list));

			/* send message */
			vortex_channel_send_rpy (channel, result, strlen (result), vortex_frame_get_msgno (frame));

			/* release memory no longer used */
			axl_free (result);
			axl_list_free (conn_list);
			return;
		}

		vortex_channel_send_rpy (channel, 
					 vortex_frame_get_payload (frame), 
					 vortex_frame_get_payload_size (frame), 
					 vortex_frame_get_msgno (frame));
	}

	return;
}


/* mod_test_15 init handler */
static int  mod_test_15_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	msg ("Test 15: calling to init module 15..");

	/* register a profile at this point */
	vortex_profiles_register (TBC_VORTEX_CTX (ctx), "urn:aspl.es:beep:profiles:reg-test:profile-15",
				  NULL, NULL, NULL, NULL, mod_test_15_frame_received, NULL);


	return axl_true;
} /* end mod_test_15_init */

/* mod_test_15 close handler */
static void mod_test_15_close (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_15_close */

/* mod_test_15 reconf handler */
static void mod_test_15_reconf (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_15_reconf */

/* mod_test_15 unload handler */
static void mod_test_15_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_15_unload */

/* mod_test_15 ppath-selected handler */
static axl_bool mod_test_15_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {

	/* notification ok */
	return axl_true;
} /* end mod_test_15_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_test_15",
	"Module used to run test_15 and check profile path function",
	mod_test_15_init,
	mod_test_15_close,
	mod_test_15_reconf,
	mod_test_15_unload,
	mod_test_15_ppath_selected
};

END_C_DECLS


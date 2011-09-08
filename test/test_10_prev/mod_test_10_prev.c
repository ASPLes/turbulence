/* mod_test_10_prev implementation */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

#define REGISTER(uri) do { \
vortex_profiles_register (TBC_VORTEX_CTX(ctx), uri, \
	NULL, NULL,  \
	NULL, NULL,  \
	NULL, NULL); \
} while(0);

void test_10_received (VortexChannel    * channel, 
		       VortexConnection * connection, 
		       VortexFrame      * frame, 
		       axlPointer         user_data)
{
	TurbulenceCtx      * ctx = user_data;
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

typedef struct _FailStructure  {
	char * value;
} FailStructure;

void test_10_a_received (VortexChannel    * channel, 
			 VortexConnection * connection, 
			 VortexFrame      * frame, 
			 axlPointer         user_data)
{
	FailStructure * structure = user_data;

	/*** begin: FORCED SEG FAULT ACCESS ***/
	printf ("This will fail: %s\n", structure->value);
	/*** end: FORCED SEG FAULT ACCESS ***/

	return;
}

/* mod_test_10_prev init handler */
static int  mod_test_10_prev_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	msg ("Test 10_prev: calling to init module 10_prev..");

	/* register here all profiles required by tests */
	REGISTER ("urn:aspl.es:beep:profiles:reg-test:profile-1");
	REGISTER ("urn:aspl.es:beep:profiles:reg-test:profile-2");
	REGISTER ("urn:aspl.es:beep:profiles:reg-test:profile-3");
	REGISTER ("urn:aspl.es:beep:profiles:reg-test:profile-4");

	/* register a frame received for the remote side (child process) */
	vortex_profiles_set_received_handler (TBC_VORTEX_CTX(ctx), "urn:aspl.es:beep:profiles:reg-test:profile-1", 
					      test_10_received, ctx);
	vortex_profiles_set_received_handler (TBC_VORTEX_CTX(ctx), "urn:aspl.es:beep:profiles:reg-test:profile-2", 
					      test_10_received, ctx);

	/* register here all profiles required by tests */
	REGISTER("urn:aspl.es:beep:profiles:reg-test:profile-1-failed");

	/* register a frame received for the remote side (child process) */
	vortex_profiles_set_received_handler (TBC_VORTEX_CTX(ctx), "urn:aspl.es:beep:profiles:reg-test:profile-1-failed", 
					      test_10_a_received, NULL);

	return axl_true;
} /* end mod_test_10_prev_init */

/* mod_test_10_prev close handler */
static void mod_test_10_prev_close (TurbulenceCtx * _ctx) {

	return;
} /* end mod_test_10_prev_close */

/* mod_test_10_prev reconf handler */
static void mod_test_10_prev_reconf (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_10_prev_reconf */

/* mod_test_10_prev unload handler */
static void mod_test_10_prev_unload (TurbulenceCtx * _ctx) {
	return;
} /* end mod_test_10_prev_unload */

/* mod_test_10_prev ppath-selected handler */
static axl_bool mod_test_10_prev_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {

	/* notification ok */
	return true;
} /* end mod_test_10_prev_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_test_10_prev",
	"Module used to run test_10_prev and check profile path function",
	mod_test_10_prev_init,
	mod_test_10_prev_close,
	mod_test_10_prev_reconf,
	mod_test_10_prev_unload,
	mod_test_10_prev_ppath_selected
};

END_C_DECLS


/* mod_websocket implementation */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* mod_websocket init handler */
static int  mod_websocket_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* Place here your mod init code. This will be called once turbulence decides to include the module. */
	return axl_true;
} /* end mod_websocket_init */

/* mod_websocket close handler */
static void mod_websocket_close (TurbulenceCtx * _ctx) {
	/* Place here the code required to stop and dealloc resources used by your module */

	return;
} /* end mod_websocket_close */

/* mod_websocket reconf handler */
static void mod_websocket_reconf (TurbulenceCtx * _ctx) {
	/* Place here all your optional reconf code if the HUP signal is received */

	return;
} /* end mod_websocket_reconf */

/* mod_websocket unload handler */
static void mod_websocket_unload (TurbulenceCtx * _ctx) {
	/* Place here the code required to dealloc resources used by
	 * your module because turbulence signaled the child process
	 * must not have access */
	return;
} /* end mod_websocket_unload */

/* mod_websocket ppath-selected handler */
static axl_bool mod_websocket_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {
	/* Place here the code to implement all provisioning that was
	 * deferred because non enough data was available at init
	 * method (connection and profile path selected) */
	return axl_true;
} /* end mod_websocket_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_websocket",
	"Support for BEEP over WebSocket (through libvortex-websocket-1.1)",
	mod_websocket_init,
	mod_websocket_close,
	mod_websocket_reconf,
	mod_websocket_unload,
	mod_websocket_ppath_selected
};

END_C_DECLS


/* mod_test_11 implementation */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* mod_test_11 init handler */
static int  mod_test_11_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

/* nothing to do here */
return axl_true;
} /* end mod_test_11_init */

/* mod_test_11 close handler */
static void mod_test_11_close (TurbulenceCtx * _ctx) {
/* no close code */
} /* end mod_test_11_close */

/* mod_test_11 reconf handler */
static void mod_test_11_reconf (TurbulenceCtx * _ctx) {
/* no reconf code */
} /* end mod_test_11_reconf */

/* mod_test_11 unload handler */
static void mod_test_11_unload (TurbulenceCtx * _ctx) {
/* no unload code */
} /* end mod_test_11_unload */

/* mod_test_11 ppath-selected handler */
static void mod_test_11_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {

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


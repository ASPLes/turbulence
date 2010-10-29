/* mod_sasl_mysql implementation */
#include <turbulence.h>

/* mysql flags */
#include <mysql.h>

/* include support for common-sasl */
#include <common-sasl.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;



axlPointer mod_sasl_mysql_format_handler (TurbulenceCtx    * ctx,
					  SaslAuthBackend  * sasl_backend,
					  axlNode          * auth_db_node_conf,
					  ModSaslOpType      op_type,
					  const char       * auth_id,
					  const char       * authorization_id,
					  const char       * password,
					  const char       * serverName,
					  const char       * sasl_method,
					  axlError        ** err,
					  VortexMutex      * mutex)
{
	MYSQL * conn;

	switch (op_type) {
	case MOD_SASL_OP_TYPE_AUTH:
		/* request to auth user */
		break;
	case MOD_SASL_OP_TYPE_LOAD_AUTH_DB:
		/* request to load msyql database */
		
		/* connect */
		
		conn = mysql_init (NULL);
		mysql_close(conn);

		break;
	}
	return NULL;
}

/* mod_sasl_mysql init handler */
static int  mod_sasl_mysql_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* install here all support to handle "mysql" databases */
	return common_sasl_register_format (_ctx, "mysql", mod_sasl_mysql_format_handler);
} /* end mod_sasl_mysql_init */

/* mod_sasl_mysql close handler */
static void mod_sasl_mysql_close (TurbulenceCtx * _ctx) {
	/* Place here the code required to stop and dealloc resources used by your module */
	return;
} /* end mod_sasl_mysql_close */

/* mod_sasl_mysql reconf handler */
static void mod_sasl_mysql_reconf (TurbulenceCtx * _ctx) {
	/* Place here all your optional reconf code if the HUP signal is received */
	return;
} /* end mod_sasl_mysql_reconf */

/* mod_sasl_mysql unload handler */
static void mod_sasl_mysql_unload (TurbulenceCtx * _ctx) {
	/* Place here the code required to dealloc resources used by your module because turbulence signaled the child process must not have access */
	return;
} /* end mod_sasl_mysql_unload */

/* mod_sasl_mysql ppath-selected handler */
static axl_bool mod_sasl_mysql_ppath_selected (TurbulenceCtx * _ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {
	/* Place here the code to implement all provisioning that was deferred because non enough data was available at init method (connection and profile path selected) */
	return axl_true;
} /* end mod_sasl_mysql_ppath_selected */

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_sasl_mysql",
	"MySQL authentication backend for MOD-SASL",
	mod_sasl_mysql_init,
	mod_sasl_mysql_close,
	mod_sasl_mysql_reconf,
	mod_sasl_mysql_unload,
	mod_sasl_mysql_ppath_selected
};

END_C_DECLS


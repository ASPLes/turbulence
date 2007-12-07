/*  Turbulence:  BEEP application server
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
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
 *  at COPYING file. This is LGPL software: you are wellcome to
 *  develop propietary applications using this library withtout any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc.
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

#include <turbulence.h>

/** 
 * @internal Init for all exarg functions provided by the turbulence
 * command line.
 */
bool main_init_exarg (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install default debug options. */
	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Makes all log produced by the application, to be also dropped to the console in sort form.");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified.");

	/* install exarg options */
	exarg_install_arg ("config", "f", EXARG_STRING, 
			   "Main server configuration location.");

	exarg_install_arg ("vortex-debug", NULL, EXARG_NONE,
			   "Enable vortex debug");

	exarg_install_arg ("vortex-debug2", NULL, EXARG_NONE,
			   "Enable second level vortex debug");

	exarg_install_arg ("vortex-debug-color", NULL, EXARG_NONE,
			   "Makes vortex debug to be done using colors according to the message type. If this variable is activated, vortex-debug variable is activated implicitly.");

	exarg_install_arg ("conf-location", NULL, EXARG_NONE,
			   "Allows to get the location of the turbulence configuration file that will be used by default");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* activate log console */
	ctx->console_debug       = exarg_is_defined ("debug");
	ctx->console_debug2      = exarg_is_defined ("debug2");
	ctx->console_debug3      = exarg_is_defined ("debug3");
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* implicitly activate third and second console debug */
	if (ctx->console_debug3)
		ctx->console_debug2 = true;
	if (ctx->console_debug2)
		ctx->console_debug = true;
	
	ctx->console_color_debug = exarg_is_defined ("color-debug");

	/* check for conf-location option */
	if (exarg_is_defined ("conf-location")) {
		printf ("VERSION:     %s\n", VERSION);
		printf ("SYSCONFDIR:  %s\n", SYSCONFDIR);
		printf ("TBC_DATADIR: %s\n", TBC_DATADIR);
		printf ("Default configuration file: %s/turbulence/turbulence.conf", SYSCONFDIR);
		return false;
	}	

	/* exarg properly configured */
	return true;
}

int main (int argc, char ** argv)
{
	char          * config;
	TurbulenceCtx * ctx;

	/*** init exarg library ***/
	if (! main_init_exarg (argc, argv))
		return -1;

	/* configure lookup domain, and load configuration file */
	vortex_support_add_domain_search_path_ref (axl_strdup ("turbulence-conf"), 
						   vortex_support_build_filename (SYSCONFDIR, "turbulence", NULL));
	vortex_support_add_domain_search_path     ("turbulence-conf", ".");

	/* find the configuration file */
	if (exarg_is_defined ("config")) {
		/* get the configuration defined at the command line */
		config = axl_strdup (exarg_get_string ("config"));
	} else {
		/* get the default configuration defined at
		 * compilation time */
		config = vortex_support_domain_find_data_file ("turbulence-conf", "turbulence.conf");
	} /* end if */

	/* load main turb */
	if (config == NULL)
		error ("Unable to find turbulence.conf file at the default location: %s/turbulence/turbulence.conf", SYSCONFDIR);
	else 
		msg ("using configuration file: %s", config);

	/* create the turbulence context */
	ctx = turbulence_ctx_new ();

	/* init libraries */
	if (! turbulence_init (ctx)) {
		/* free turbulence ctx */
		turbulence_ctx_free (ctx);
		return -1;
	} /* end if */

	/* not required to free config var, already done by previous
	 * function */
	msg ("about to startup configuration found..");
	if (! turbulence_run_config (ctx))
		return false;

	/* look main thread until finished */
	vortex_listener_wait ();
	
	/* terminate turbulence execution */
	turbulence_cleanup (ctx, 0);

	/* free context */
	turbulence_ctx_free (ctx);
	
	return 0;
}

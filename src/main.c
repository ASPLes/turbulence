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
 *         info@aspl.es - http://fact.aspl.es
 */

#include <turbulence.h>

int main (int argc, char ** argv)
{
	char * config;

	/* init libraries */
	turbulence_init (argc, argv);

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
	msg ("using configuration file: %s", config);
	if (! turbulence_config_load (config)) {
		/* unable to load configuration */
		return -1;
	}

	/* rest of modules to initialize */
	turbulence_log_init ();
	
	/* not required to free config var, already done by previous
	 * function */
	msg ("about to startup configuration found..");
	if (! turbulence_run_config ())
		return false;

	/* init profile path */
	if (! turbulence_ppath_init ())
		return false;

	/* look main thread until finished */
	vortex_listener_wait ();
	
	/* terminate turbulence execution */
	turbulence_cleanup (0);
	
	return 0;
}

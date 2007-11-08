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
 * @internal By default clean start is disabled.
 */
bool turbulence_clean_start = false;

#define CLEAN_START() do{error ("Clean start activated, stopping turbulence due to a startup failure found"); turbulence_exit (-1);exit (-1);}while (0);

/** 
 * @brief Tries to load all modules found at the directory already
 * located. In fact the function searches for xml files that points to
 * modules to be loaded.
 * 
 * @param dirHandle The directory that will be inspected for modules.
 */
void turbulence_run_load_modules_from_path (const char * path, DIR * dirHandle, axlDtd * dtd)
{
	struct dirent    * entry;
	char             * fullpath = NULL;
	axlDoc           * doc;
	axlError         * error;
	TurbulenceModule * module;
	ModInitFunc        init;
				      


	/* get the first entry */
	entry = readdir (dirHandle);
	while (entry != NULL) {
		/* nullify full path and doc */
		fullpath = NULL;
		doc      = NULL;
		error    = NULL;

		/* check for non valid directories */
		if (axl_cmp (entry->d_name, ".") ||
		    axl_cmp (entry->d_name, "..")) {
			goto next;
		} /* end if */
		
		fullpath = vortex_support_build_filename (path, entry->d_name, NULL);
		if (! vortex_support_file_test (fullpath, FILE_IS_REGULAR))
			goto next;

		/* notify file found */
		msg ("file found: %s", fullpath);		

#if defined(AXL_OS_UNIX)
		/* check if the fullpath is ended with a ~ sign */
		if (fullpath [strlen (fullpath) - 1] == '~') {
			wrn ("skiping file %s which looks a backup one", fullpath);
			goto next;
		} /* end if */
#endif

		/* check its xml format */
		doc = axl_doc_parse_from_file (fullpath, &error);
		if (doc == NULL) {
			wrn ("file %s does not appear to be a valid xml file (%s), skipping..", fullpath, axl_error_get (error));
			goto next;
		}
		
		/* now validate the file found */
		if (! axl_dtd_validate (doc, dtd, &error)) {
			wrn ("file %s does is not a valid turbulence module pointer: %s", fullpath,
			     axl_error_get (error));
			goto next;
		} /* end if */

		msg ("loading mod turbulence pointer: %s", fullpath);

		/* load the module man!!! */
		module = turbulence_module_open (ATTR_VALUE (axl_doc_get_root (doc), "location"));
		if (module == NULL) {
			wrn ("unable to open module: %s", ATTR_VALUE (axl_doc_get_root (doc), "location"));
			goto next;
		} /* end if */

		/* init the module */
		init = turbulence_module_get_init (module);
		
		/* check init */
		if (! init ()) {
			wrn ("init module: %s have failed, skiping", ATTR_VALUE (axl_doc_get_root (doc), "location"));
			CLEAN_START();
		} else {
			msg ("init ok, registering module: %s", ATTR_VALUE (axl_doc_get_root (doc), "location"));
		}

		/* free the document */
		axl_doc_free (doc);

		/* register the module to be loaded */
		turbulence_module_register (module);

	next:
		/* free the error */
		axl_error_free (error);

		/* free full path */
		axl_free (fullpath);

		/* get the next entry */
		entry = readdir (dirHandle);
	} /* end while */

	return;
}

/** 
 * @brief Loads all paths from the configuration, calling to load all
 * modules inside those paths.
 * 
 * @param doc The turbulence run time configuration.
 */
void turbulence_run_load_modules (axlDoc * doc, axlDtd * dtd)
{
	axlNode     * directory;
	const char  * path;
	DIR         * dirHandle;

	directory = axl_doc_get (doc, "/turbulence/modules/directory");
	if (directory == NULL) {
		msg ("no module directories were configured, nothing loaded");
		return;
	}

	/* check every module */
	while (directory != NULL) {
		/* get the directory */
		path = ATTR_VALUE (directory, "src");
		
		/* try to open the directory */
		dirHandle = opendir (path);
		if (dirHandle == NULL) {
			wrn ("unable to open mod directory '%s' (%s), skiping to the next", strerror (errno), path);
			goto next;
		} /* end if */

		/* directory found, now search for modules activated */
		msg ("found mod directory: %s", path);
		turbulence_run_load_modules_from_path (path, dirHandle, dtd);
		
		/* close the directory handle */
		closedir (dirHandle);
	next:
		/* get the next directory */
		directory = axl_node_get_next_called (directory, "directory");
		
	} /* end while */
}

/** 
 * @brief Takes current configuration, and starts all settings
 * required to run the server.
 * 
 * Later, all modules will be loaded adding profile configuration.
 *
 * @return false if the function is not able to properly start
 * turbulence or the configuration will produce bad results.
 */
bool turbulence_run_config    ()
{
	/* get the document configuration */
	axlDoc           * doc = turbulence_config_get ();
	axlNode          * port;
	axlNode          * listener;
	axlNode          * name;
	axlNode          * node;
	VortexConnection * con_listener;

	/* mod turbulence dtd */
	char             * dtd_file;
	char             * string_aux;
	int                int_aux;
	axlDtd           * dtd;
	axlError         * error;
	bool               at_least_one_listener = false;

	/* check clean start */
	node                   = axl_doc_get (doc, "/turbulence/global-settings/clean-start");
	turbulence_clean_start = (HAS_ATTR_VALUE (node, "value", "yes"));

	/* configure max connection settings here */
	node       = axl_doc_get (doc, "/turbulence/global-settings/connections/max-connections");

	/* set hard limit */
	string_aux = (char*) ATTR_VALUE (node, "hard-limit");
	int_aux    = strtol (string_aux, NULL, 10);
	if (! vortex_conf_set (VORTEX_HARD_SOCK_LIMIT, int_aux, NULL)) {
		error ("failed to set hard limit to=%s (int value=%d), terminating turbulence..",
		       string_aux, int_aux);
		return false;
	} /* end if */
	msg ("configured max connections hard limit to: %s", string_aux);

	/* set soft limit */
	string_aux = (char*) ATTR_VALUE (node, "soft-limit");
	int_aux    = strtol (string_aux, NULL, 10);
	if (! vortex_conf_set (VORTEX_SOFT_SOCK_LIMIT, int_aux, NULL)) {
		error ("failed to set soft limit to=%s (int value=%d), terminating turbulence..",
		       string_aux, int_aux);
		return false;
	} /* end if */
	msg ("configured max connections soft limit to: %s", string_aux);

	/* check log configuration */
	node = axl_doc_get (doc, "/turbulence/global-settings/log-reporting");
	if (HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* implement log reporting */
		
	} /* end if */

	node = axl_doc_get (doc, "/turbulence/global-settings/tls-support");
	if (HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* enable sasl support */
		/* turbulence_tls_enable (); */
	} /* end if */

	/* find turbulence module dtd validation */
	dtd_file = vortex_support_domain_find_data_file ("turbulence-data", "mod-turbulence.dtd");
	if (dtd_file == NULL) {
		/* free document */
		error ("unable to find mod turbulence DTD definition (mod-turbulence.dtd), check your turbulence installation.");
		turbulence_exit (-1);
		return false;
	} /* end if */
 
	/* found dtd file */
	msg ("found dtd file at: %s", dtd_file);
	dtd = axl_dtd_parse_from_file (dtd_file, &error);
	if (dtd == NULL) {
		error ("unable to load mod-turbulence.dtd file: %s", axl_error_get (error));
		
		axl_error_free (error);
		turbulence_exit (-1);
		
		return false;
	}

	/* now load all modules found */
	turbulence_run_load_modules (doc, dtd);
	
	/* free dtds */
	axl_free (dtd_file);
	axl_dtd_free (dtd);
	

	/* now check for profiles already activated */
	if (vortex_profiles_registered () == 0) {
		error ("unable to start turbulence server, no profile was registered into the vortex engine either by configuration or modules");
		turbulence_exit (-1);
		return false;
	} /* end if */
	
	/* get the first listener configuration */
	listener = axl_doc_get (doc, "/turbulence/global-settings/listener");
	while (listener != NULL) {
		/* get the listener name configuration */
		name = axl_node_get_child_called (listener, "name");
		
		/* get ports to be allocated */
		port = axl_doc_get (doc, "/turbulence/global-settings/ports/port");
		while (port != NULL) {

			/* start the listener */
			con_listener = vortex_listener_new (
				/* listener name */
				axl_node_get_content (name, NULL),
				/* port to use */
				axl_node_get_content (port, NULL),
				/* on ready callbacks */
				NULL, NULL);
			
			/* check the listener started */
			if (! vortex_connection_is_ok (con_listener, false)) {
				/* unable to start the server configuration */
				error ("unable to start listener at %s:%s...", 
				     
				     /* server name */
				     axl_node_get_content (name, NULL),
				     
				     /* server port */
				     axl_node_get_content (port, NULL));

				/* check clean start */ 
				CLEAN_START ();

				goto next;
			} /* end if */

			msg ("started listener at %s:%s...",
			     axl_node_get_content (name, NULL),
			     axl_node_get_content (port, NULL));

			/* flag that at least one listener was
			 * created */
			at_least_one_listener = true;
		next:
			/* get the next port */
			port = axl_node_get_next_called (port, "port");
			
		} /* end while */
		
		/* get the next listener */
		listener = axl_node_get_next_called (listener, "listener");

	} /* end if */

	if (! at_least_one_listener) {
		error ("Unable to start turbulence, no listener configuration was started, either due to configuration error or to startup problems. Terminating..");
		turbulence_exit (-1);
		return false;
	}

	/* turbulence started properly */
	return true;
}

/** 
 * @brief Allows to cleanup the module.
 */
void turbulence_run_cleanup ()
{
	return;
}



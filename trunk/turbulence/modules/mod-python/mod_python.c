/*  Turbulence BEEP application server
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions, supporting
 *  turbulence based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/turbulence
 */

/* include python headers */
#undef _GNU_SOURCE /* make Python.h happy */
#include <Python.h>

/* mod_python implementation */
#include <turbulence.h>

/* include py-turbulence */
#include <py_turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

/* global turbulence context reference */
TurbulenceCtx * ctx = NULL;

/* reference to the last configuration file loaded */
axlDoc       * mod_python_conf = NULL;
/* reference to the last site configuration file loaded */
axlDoc       * mod_python_site_conf = NULL;

/* mutex used to control application top level initialization */
VortexMutex    mod_python_top_init;
/* control if python itself was initialized on this process */
axl_bool       mod_python_py_init = axl_false;

#define APPLICATION_LOAD_FAILED(msg, ...)                                                      \
	/* check to close the connection on failure */                                         \
	if (conn && turbulence_config_is_attr_positive (ctx, node, "close-conn-on-failure")) { \
	      TBC_FAST_CLOSE(conn);                                                            \
        }                                                                                      \
        wrn (msg, ##__VA_ARGS__);                                                              \
        /* clean for clean start activated */                                                  \
        CLEAN_START (ctx);                                                                     \
                                                                                               \
        /* get next node */                                                                    \
        node = axl_node_get_next_called (node, "application");                                 \
        continue;                                                 

/** 
 * @brief Allows to add the provided path into the current sys.path
 * list. This is used to allow loading user defined python code
 * (applications) from locations not registered by the system.
 */ 
axl_bool mod_python_add_path (const char * string) 
{
	PyObject * module = PyImport_ImportModule ("sys");
	PyObject * str    = NULL;
	PyObject * path   = NULL;
	axl_bool   result = axl_false;

	if (module == NULL) {
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("Failed to add path %s into sys.path, unable to load sys module", string);
		goto finish;
	} /* end if */
	
	/* get path symbol */
	path = PyObject_GetAttrString (module, "path");
	if (path == NULL) {
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("Failed to add path %s into sys.path, unable to load path component from sys", string);
		goto finish;
	} /* end if */

	/* build pythong string path */
	str = PyString_FromString (string);
	if (PyList_Insert (path, 0, str)) {
		wrn ("Error: unable to add %s to the path..", string);
		goto finish;
	} /* end if */

	/* return proper status */
	result = axl_true;
finish:
	Py_XDECREF (module);
	Py_XDECREF (path);
	Py_XDECREF (str);

	return result;
}

/** 
 * @brief Removes the first path added into the sys.path
 */ 
axl_bool mod_python_remove_first_path (void) 
{
	PyObject * module   = PyImport_ImportModule ("sys");
	PyObject * str      = NULL;
	PyObject * path     = NULL;
	PyObject * new_path = NULL;
	axl_bool   result   = axl_false;

	if (module == NULL) {
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("Failed to remove first path from sys.path, unable to load sys module");
		goto finish;
	} /* end if */
	
	/* get path symbol */
	path = PyObject_GetAttrString (module, "path");
	if (path == NULL) {
		py_vortex_handle_and_clear_exception (NULL);

		wrn ("Failed to remove first path sys.path, unable to load path component from sys");
		goto finish;
	} /* end if */

	/* get new path */
	new_path = PyList_GetSlice (path, 1, PyList_Size (path));

	/* now set it */
	if (PyObject_SetAttrString (module, "path", new_path)) {
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("Failed to set new path (removing the first path from sys.path)");
		goto finish;
	}

	/* return proper status */
	result = axl_true;
finish:
	Py_XDECREF (module);
	Py_XDECREF (path);
	Py_XDECREF (str);
	Py_XDECREF (new_path);

	return result;
}

/** 
 * @brief Function used to get inside the python application code
 * calling the init function.
 */ 
axl_bool mod_python_init_app (TurbulenceCtx * ctx, PyObject * init_function)
{
	/* init function has one parameter: PyTurbulencCtx */
	PyObject * py_tbc_ctx;
	PyObject * args;
	PyObject * result;
	axl_bool   _result = axl_false;

	/* check if init_function is callable */
	if (! PyCallable_Check(init_function)) {
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("Received an app-init method reference which is not callable");
		return axl_false;
	}

	/* wrap and create turbulence.Ctx object */
	py_tbc_ctx = py_turbulence_ctx_create (ctx);

	/* not required to acquire the GIL we are calling from the
	 * thread that called to Py_Initialize */

	/* build parameters */
	args = PyTuple_New (1);
	PyTuple_SetItem (args, 0, py_tbc_ctx);

	/* now call to the function */
	msg ("calling python app init function: %p (tbc ref: %p:%d)", init_function, py_tbc_ctx, py_tbc_ctx->ob_refcnt);
	result = PyObject_Call (init_function, args, NULL);

	/* handle exceptions */
	py_vortex_handle_and_clear_exception (NULL);

	/* now parse results to know if we have properly loaded the
	 * python extension */
	if (result) {
		/* now implement other attributes */
		if (! PyArg_Parse (result, "i", &_result)) {
			py_vortex_handle_and_clear_exception (NULL);
			_result = axl_false;
		}
	} /* end if */

	Py_XDECREF (result);
	Py_XDECREF (args);

	/* prepare turbulence object */
	return _result;
}

/** 
 * @brief Loads all python applications defined the mod-python
 * configuration file. The function returns axl_true if the module can
 * continue signaling its proper start up.
 */
axl_bool mod_python_init_applications (TurbulenceCtx     * ctx, 
				       axlDoc            * python_conf,
				       const char        * workDir, 
				       const char        * serverName, 
				       VortexConnection  * conn)
{
	axlNode    * node;
	axlNode    * aux;
	axlNode    * location;
	PyObject   * module;
	PyObject   * function;
	const char * start_file;
	const char * src;

	/* for each application */
	node = axl_doc_get (python_conf, "/mod-python/application");
	while (node) {
		msg ("loading python app [%s]", ATTR_VALUE (node, "name"));

		/* check if node has serverName attribute and compare
		   it with serverName defined (and only if defined) */
		if (serverName && HAS_ATTR (node, "serverName") && ! HAS_ATTR_VALUE (node, "serverName", serverName)) {
			msg ("  -> no match for serverName=%s (app serverName=%s)", serverName, ATTR_VALUE (node, "serverName"));
			/* found serverName defined, but application spec states other serverName, skipping this application */
			/* get next node */
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */

		/* now check for applications already loaded */
		if (PTR_TO_INT (axl_node_annotate_get (node, "app-started", axl_false))) {
			msg ("  app [%s] already started", ATTR_VALUE (node, "name"));
			/* found application already loaded */
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */
		msg ("  app [%s] not started (flag not found)", ATTR_VALUE (node, "name"));
		
		/* find location node */
		location = axl_node_get_child_called (node, "location");
		if (location == NULL) {
			/* log error and continue */
			APPLICATION_LOAD_FAILED ("unable to find location configuration for python app name %s", ATTR_VALUE (node, "name"));
		}

		/* check which src to use */
		if (HAS_ATTR (location, "src"))
			src = ATTR_VALUE (location, "src");
		else
			src = workDir;

		/* check source lengths */
		if (src == NULL || strlen (src) == 0) {
			wrn ("Unable to load python app, source location is not defined and work-dir is empty");
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */

		/* drop a log */
		msg ("importing code found at: %s%s%s", src, VORTEX_FILE_SEPARATOR, ATTR_VALUE (location, "start-file"));

		/* add module path to complete import operation */
		if (! mod_python_add_path (src)) {
			APPLICATION_LOAD_FAILED ("unable to add sys.path %s to load module %s",
						 src, ATTR_VALUE (location, "name"));
		} /* end if */

		/* check if <location> node has add-path childs  */
		aux = axl_node_get_child_called (location, "add-path");
		while (aux) {
			/* add python path */
			if (! mod_python_add_path (ATTR_VALUE (aux, "value"))) {
				APPLICATION_LOAD_FAILED ("unable to add sys.path %s to load module %s",
							 ATTR_VALUE (aux, "value"), ATTR_VALUE (location, "name"));
			}
			msg ("added python path: %s", ATTR_VALUE (aux, "value"));

			/* go to the next add-path declaration */
			aux = axl_node_get_next_called (aux, "add-path");
		} /* end while */

		/* check import code do not end it .py or .pyc */
		start_file = ATTR_VALUE (location, "start-file");
		if (axl_cmp (start_file + strlen (start_file) - 3, ".py")) {
			wrn ("Start file cannot be ended with .py. This will fail. Try to leave start file without extension");
		}

		/* now load python source */
		module = PyImport_ImportModule (start_file);
		
		if (module == NULL) {
			/* handle possible exception */
			py_vortex_handle_and_clear_exception (NULL);

			APPLICATION_LOAD_FAILED ("Failed to import module %s located at %s", 
						 start_file, src);
		} /* end if */

		msg ("module %s (%p) load ok", start_file, module);

		/* ok, now load the entry point, app-close and app-reload */
		function = PyObject_GetAttrString (module, ATTR_VALUE (location, "app-init"));
		if (function == NULL) {
			py_vortex_handle_and_clear_exception (NULL);

			APPLICATION_LOAD_FAILED ("Failed to load app-init %s function inside module %s", 
						 ATTR_VALUE (location, "app-init"), start_file);
		}
		msg ("app-init %s (%p) entry point found", ATTR_VALUE (location, "app-init"), function);

		/* call to activate module */
		if (! mod_python_init_app (ctx, function)) {

			/* remove path */
			if (! mod_python_remove_first_path ()) {
				APPLICATION_LOAD_FAILED ("Failed to start application %s:%s init function failed, but also found failure to remove path from sys.path",
							 start_file, ATTR_VALUE (location, "app-init"));
			} /* end if */

			APPLICATION_LOAD_FAILED ("Failed to start application %s:%s, init function failed", 
						 start_file, ATTR_VALUE (location, "app-init"));
		} /* end if */

		/* record the module into the application node
		 * configuration to use its reference for later
		 * reconfiguration and close notification */
		axl_node_annotate_data (node, "module", module);
		msg ("stored module reference %p (%s)", module, ATTR_VALUE (node, "name"));

		/* remove path only if kee-path is not set to yes */
		if (! HAS_ATTR_VALUE (node, "keep-path", "yes")) {
			if (! mod_python_remove_first_path ()) {
				APPLICATION_LOAD_FAILED ("Failed to remove path from sys.path after python app initialization",
							 start_file, ATTR_VALUE (location, "app-init"));
			} /* end if */
		} /* end if */

		/* get close method here */
		function = PyObject_GetAttrString (module, ATTR_VALUE (location, "app-close"));
		msg ("close reference status %s: %p", ATTR_VALUE (location, "app-close"), function);

		/* flag this application as loaded */
		axl_node_annotate_data (node, "app-started", INT_TO_PTR (axl_true));

		/* get next node */
		node = axl_node_get_next_called (node, "application");
	}
	return axl_true;
}

void mod_python_exception (const char * exception_msg)
{
	/* log into turbulence error found */
	error (exception_msg);
	
	return;
}

/* mod_python init handler */
static int  mod_python_init (TurbulenceCtx * _ctx) {
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* init mutex */
	vortex_mutex_create (&mod_python_top_init);
	mod_python_py_init = axl_false;

	return axl_true;
} /* end mod_python_init */

void mod_python_close_module (axlNode * node, axlNode * location)
{
	PyObject           * module = axl_node_annotate_get (node, "module", axl_false);
	PyObject           * close  = NULL;
	PyObject           * result = NULL;

	if (module == NULL) {
		wrn ("module reference is null");
		return;
	}
	
	msg ("calling to notify close on module ref: %p (%s:%s)", 
	     module, ATTR_VALUE (node, "name"), PyModule_GetName (module));

	/* get close method */
	close = PyObject_GetAttrString (module, ATTR_VALUE (location, "app-close"));
	if (close == NULL) {
		/* handle exception */
		py_vortex_handle_and_clear_exception (NULL);
		wrn ("unable to find close handler ('%s') for module %s: did you define it?",
		     ATTR_VALUE (location, "app-close"), ATTR_VALUE (node, "name"));

		return;
	}

	/* call to notify close */
	result = PyObject_Call (close, PyTuple_New (0), NULL);

	msg ("application close handler for %s called", ATTR_VALUE (node, "name"));
	
	/* handle exceptions */
	py_vortex_handle_and_clear_exception (NULL);
	
	/* terminate result */
	Py_XDECREF (result);
	/* do not terminate module with Py_DECREF: this is already
	 * done by mod_python_module_unref */

	return;
}

/* mod_python close handler */
static void mod_python_close (TurbulenceCtx * _ctx) {
	axlNode           * node;
	axlNode           * location;
	PyGILState_STATE    state;

	/* destroy */
	vortex_mutex_destroy (&mod_python_top_init);

	/* check if the module was initialized */
	if (! mod_python_py_init)
		return;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* call to notify close on all apps */
	node = axl_doc_get (mod_python_conf, "/mod-python/application");
	while (node) {

		/* check for initialized applications */
		if (! PTR_TO_INT (axl_node_annotate_get (node, "app-started", axl_false))) {
			/* found application already loaded */
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */

		/* find location node */
		location = axl_node_get_child_called (node, "location");
		if (location == NULL) {
			continue;
		} /* end if */

		/* call to notify close */
		mod_python_close_module (node, location);

		/* get next node */
		node = axl_node_get_next_called (node, "application");
	}

	msg ("mod_python_close: apps close notification finished..");

	/* terminate configuration */
	axl_doc_free (mod_python_conf);
	mod_python_conf = NULL;

	axl_doc_free (mod_python_site_conf);
	mod_python_site_conf = NULL;

	/* now defer python finalize call to avoid calling to py
	   finalize having references setup inside turbulence and
	   vortex structures. This will ensure this finalize is called
	   as late as possible. */
	Py_Finalize ();


	/* not required to release the GIL */
	return;

} /* end mod_python_close */

/* mod_python reconf handler */
static void mod_python_reconf (TurbulenceCtx * _ctx) {
	/* received reconf signal */
	return;
} /* end mod_python_reconf */

/** 
 * @internal Check and initialize python engine.
 */
void mod_python_initialize (void)
{
	if (! mod_python_py_init) {
		msg ("Initializing python engine..");
		/* initialize python */
		Py_Initialize ();

		/* call to initialize threading API and to acquire the lock */
		PyEval_InitThreads();

		/* call to init py-turbulence */
		py_turbulence_init ();

		/* configure exception handler */
		py_vortex_set_exception_handler (mod_python_exception);

		/* signal python initialized */
		mod_python_py_init = axl_true;
	} else {
		msg ("Python engine already initialized..");
	} /* end if */
	return;
}

/** 
 * @internal Check and load mod-python configuration (python.conf).
 */
axl_bool mod_python_load_config (void) {
	char     * config;
	axlError * error = NULL;

	if (mod_python_conf == NULL) {
		/* python.conf not loaded */
		vortex_support_add_domain_search_path_ref (TBC_VORTEX_CTX(ctx), axl_strdup ("python"), 
							   vortex_support_build_filename (turbulence_sysconfdir (ctx), "turbulence", "python", NULL));

		/* load configuration file */
		config           = vortex_support_domain_find_data_file (TBC_VORTEX_CTX(ctx), "python", "python.conf");
		msg ("loading mod-python config: %s", config);
		mod_python_conf  = axl_doc_parse_from_file (config, &error);
		axl_free (config);
		if (mod_python_conf == NULL) {
			error ("failed to load mod-python configuration file, error found was: %s", 
			       axl_error_get (error));
			axl_error_free (error);
			return axl_false;
		} /* end if */
	} else {
		msg ("mod-python config already loaded..");
	} /* end if */

	return axl_true;
}

axl_bool mod_python_load_site_config (TurbulenceCtx    * ctx, 
				      const char       * workDir, 
				      const char       * serverName,
				      VortexConnection * conn) 
{

	char             * site_config;
	axlError         * err = NULL;

	/* check if site config was not loaded */
	if (mod_python_site_conf == NULL) {

		site_config = axl_strdup_printf ("%s/python.conf", workDir);
		msg ("Checking to load site python.conf at %s", site_config);
		if (vortex_support_file_test (site_config, FILE_EXISTS)) {
			
			msg ("Found site %s/python.conf, loading..", workDir);
			mod_python_site_conf  = axl_doc_parse_from_file (site_config, &err);
			/* check parse result */
			if (mod_python_site_conf == NULL) {
				error ("failed to load mod-python site configuration file at %s, error found was: %s", 
				       site_config, axl_error_get (err));
				axl_free (site_config);
				axl_error_free (err);
				return axl_false;
			} /* end if */

		} /* end if */
		axl_free (site_config);
	} /* end if */

	/* now init applications that may not be already initialized */
	if (! mod_python_init_applications (ctx, mod_python_site_conf, workDir, serverName, conn)) 
		return axl_false;

	return axl_true;
}

/** 
 * @brief Starts python function once a profile path is selected 
 */
static axl_bool mod_python_ppath_selected (TurbulenceCtx      * ctx, 
					   TurbulencePPathDef * ppath_selected, 
					   VortexConnection   * conn) {

	const char       * serverName;

	/* get work directory and serverName */
	const char       * workDir;
	PyGILState_STATE   state; 

	serverName = turbulence_ppath_get_server_name (conn);
	workDir    = turbulence_ppath_get_work_dir (ctx, ppath_selected);

	/* lock to check and initialize */
	vortex_mutex_lock (&mod_python_top_init);

	if (! mod_python_py_init) {
		/* initialize python engine and configure it to use with
		   mod-python */
		mod_python_initialize ();

		/* acquire the GIL */
		PyEval_ReleaseLock ();
	} /* end if */

	/* acquire the GIL */
	state  = PyGILState_Ensure();

	/* check and load mod-python config */
	if (! mod_python_load_config ()) {
		/* release the GIL */
		PyGILState_Release (state); 
		vortex_mutex_unlock (&mod_python_top_init);
		return axl_false;
	}

	/* for each application found, register it and call to
	 * initialize its function */
	if (! mod_python_init_applications (ctx, mod_python_conf, workDir, serverName, conn)) {
		PyGILState_Release (state); 
		vortex_mutex_unlock (&mod_python_top_init);
		return axl_false;
	}

	/* now load python applications at the working dir if it is found */
	if (! mod_python_load_site_config (ctx, workDir, serverName, conn)) {
		PyGILState_Release (state); 
		vortex_mutex_unlock (&mod_python_top_init);
		return axl_false;
	}

	msg ("mod-python applications initialized");

	/* let other threads to enter inside python engine: this must
	   be the last call: release the GIL */
	/* PyGILState_Release(state); */
	PyGILState_Release(state); 
	vortex_mutex_unlock (&mod_python_top_init);

	msg ("mod-python started..");
	/* release lock */
	return axl_true;
}

/* Entry point definition for all handlers included in this module */
TurbulenceModDef module_def = {
	"mod_python",
	"Python server side scripting support",
	mod_python_init,
	mod_python_close,
	mod_python_reconf,
	/* unload handler */
	NULL, 
	mod_python_ppath_selected
};

END_C_DECLS

/** 
 * \page turbulence_mod_python mod-python: Turbulence python language support
 *
 * \section turbulence_mod_python_intro Introduction
 *
 * Turbulence's mod-python is a module extension that allows writing
 * server side BEEP enabled python applications to handle/implement
 * profiles. It allows writing dynamic applications that loads at run
 * time, saving the compile-install cycle required by usual C modules.
 *
 * \section turbulence_mod_python_config_location Global mod-python python.conf location
 *
 * Assuming you have a python app that works with Turbulence
 * mod-python, you can configure it under the global python.conf file
 * which is usually found at:
 *
 * \code
 * /etc/turbulence/python/python.conf
 * \endcode
 *
 * If you don't find the file, try running the following to find base
 * directory where to find config files:
 *
 * \code
 * >> turbulence --conf-location
 * \endcode
 *
 * \section turbulence_mod_python_config_location_site Site mod-python python.conf location
 *
 * You can also place a python.conf file at the profile path working
 * dir (work-dir attribute). This method is recommend in the case more
 * application separation is needed. 
 *
 * Assuming you have the following profile path declaration:
 *
 * \code
 * <path-def server-name="core-admin" 
 *           src=".*" path-name="core-admin" 
 *           separate="yes" 
 *           reuse="yes" run-as-user="core-admin" run-as-group="core-admin" 
 *           work-dir="/home/acinom/programas/core-admin/server-component" >
 *    <!-- more declarations -->
 * </path-def>
 * \endcode
 *
 * Then you can place a python.conf file with your python app initializations at:
 *
 * \code
 * /home/acinom/programas/core-admin/server-component/python.conf
 * \endcode
 *
 * \section turbulence_mod_python_configuring Configuring mod-python
 *
 * No matter where is located your python.conf, inside that file is
 * found a list of applications installed/recognized by Turbulence. It
 * should look like:
 *
 * \htmlinclude turbulence.python.xml-tmp
 * 
 * In this example we have two applications: "test app" and "core
 * admin" which are located the directories configured by <b>"src"</b>
 * attribute and started by the python entry file defined by
 * <b>"start-file"</b> attribute.
 *
 * \note To know more about writing mod-python applications see \ref turbulence_mod_python_writing_apps
 *
 * Note that each <application> may have a <b>"serverName"</b>
 * declaration configured. This lets to know mod-python to only start
 * those applications where the BEEP connection is bound to that
 * serverName value (virtual hosting). Note a BEEP connection gets
 * bounded to a serverName on first channel accepted requesting such
 * serverName.
 *
 * See following section for all details. 
 *
 * \section turbulence_mod_python_configuration_reference Configuration reference
 *
 * To declare an application, add a <application> node with the
 * following attributes:
 *
 *   - <b>name</b>: application name to better track it in logs.
 *
 *   - <b>serverName</b>: optional serverName under which the
 *       application will be served. If this value is not provided,
 *       the application will be started on turbulence start and will
 *       be available to all connections. See also \ref "execution
 *       notes" for clarifications about this. If the value is
 *       provided, python app will only be started if and only if a
 *       connection with a channel under the provided serverName is
 *       started.
 *
 *   - <b>close-conn-on-failure</b>: [yes|no] default [no]. While
 *       writing BEEP python apps it may be desirable to shutdown the
 *       connection in case mod-python fails to initialize some
 *       application that matches rather waiting for the first request
 *       to arrive. This is particularly useful if you are creating a
 *       web application.
 *
 * Now, inside an <application> declared it is required a <location>
 * node to configure where to find startup files and other
 * settings. Attributes supported by <location> node are:
 *
 *   - <b>src</b>: this is the application work directory. This path
 *       will be added to python sys.path. This path must be the
 *       directory that holds the start up file defined on
 *       <b>start-file</b> attribute.
 *
 *   - <b>start-file</b>: This is the python file that implements the
 *       set of handlers to start the application, handle reload and
 *       close the application when turbulence signal this. Note: do
 *       no place .py to this value. Avoid using "-" as part of the
 *       start file. Use "_" instead.
 *
 *   - <b>app-init</b>: handler name defined inside the <b>start-file</b> that handle python app init.
 *
 *   - <b>app-close</b>: handler name defined inside the <b>start-file</b> that handle python app close.
 *
 *   - <b>app-reload</b>: handler name defined inside the <b>start-file</b> that handle python app reload.
 *
 * \section turbulence_mod_python_execution_notes Notes about security and application isolation
 *
 * Due to python global interpreter nature, all apps started inside
 * the same process shares and access to the same data and
 * context. Obviously this has advantages and security
 * disadvantages. Knowing this, keep in mind the following
 * recommendations:
 *
 * - <b>For Personal or intranet application</b>: you can safely share
 *   application contexts without any expecial consideration
 *   (obviously you have to properly configure your profile path and
 *   protect your python app profiles with SASL and TLS).
 *
 * - <b>For several python applications</b>: were it is a concern to share
 *   context: then you have to use profile path separate=yes combined
 *   with proper serverName configuration to ensure a separate child
 *   is created for each python app. With this, each python context
 *   will run in a separted process. In this context you may also be
 *   interested in profile path chroot attribute to improve isolation level.
 *
 * - <b>For several separeted applications with shared state</b>: this
 *   is the case where it is required to share state between BEEP
 *   clients so all of them access to the same python context. To
 *   enable this configuration you'll have to combine resue="yes" and
 *   separate="yes" at profile path declaration. Again, you may also
 *   be interested in profile path chroot attribute to improve
 *   isolation level.
 *
 * 
 * \section turbulence_mod_python_add_path Adding path to python application
 *
 * It may be useful to allow including additional path from which to
 * load python code that complete your python application. For that,
 * you can use:
 * 
 * \htmlinclude python.module.example.5.xml-tmp
 * 
 */

/** 
 * \page turbulence_mod_python_writing_apps Writing Turbulence's mod-python applications
 * 
 * \section turbulence_mod_python_writing_apps_intro Introduction
 *
 * Writing a python application using mod-python module involves the
 * following steps:
 *
 *  -# Writing an startup application that fulfills a python interface (specially <b>start-file</b> attribute)
 *  -# Add some configuration to your turbulence server to detect your application (see \ref turbulence_mod_python "mod-python configuration")
 *  -# Use PyVortex (http://www.aspl.es/vortex/py-vortex/html/) and PyTurbulence (http://www.aspl.es/turbulence/py-turbulence/html/) API to handle and issue BEEP messages.
 *
 * Knowing this general terms, the following is a tutorial to develop
 * BEEP echo profile like using mod-python.
 *
 * \section turbulence_mod_python_writing_apps_skel A mod-python application skel
 *
 * <ol>
 *  <li>First, we have to create the start up file that will initialize
 *  your python application. The following is a basic skel (save content into __init__.py):
 *
 * \include python.module.example
 *  </li>
 *
 * <li>Now we have to place some initialization code to register our echo
 * profile. For that, extend app_init function to include the following:
 *
 * \include python.module.example.2
 * </li>
 *
 * <li>Ok, now we have our BEEP profile that echoes all content received
 * over a channel running <i>"urn:aspl.es:beep:profiles:echo"</i>, we need to
 * tell Turbulence to load it. This is done by adding the following
 * content into python.conf file. See \ref turbulence_mod_python
 * "mod-python administration reference" for more details.
 *
 * \htmlinclude python.module.example.3.xml-tmp
 * 
 * </li>
 *
 * <li>Now Turbulence will load our application once at the first
 * connection received, registering profile and frame received handler
 * associated. One thing remains: we have to update our \ref profile_path_configuration "profile path policy"
 * to allow our profile to be reachable.  For example,
 * we could use the following to allow executing our profile only if
 * BEEP client connects from our local network (assuming
 * 192.168.0.0/24): 
 *
 * \htmlinclude python.module.example.4.xml-tmp 
 *
 * </li>
 * </ol>
 *
 * \section turbulence_mod_python_getting_working_dir How to get current profile path working directory 
 *
 * Inside \ref profile_path_configuration "profile path configuration"
 * it is possible to define a working directory where the user can
 * place some special files that are loaded especific profile path
 * activated. In many cases it is interesting to get where is this
 * working dir located so the BEEP application can store files using
 * current running uid/gid. To do so, you can use the following:
 *
 * \code
 * # run this at the application_init python method
 * working_dir = os.path.dirname (sys.modules[__name__].__file__)
 * \endcode
 *
 */

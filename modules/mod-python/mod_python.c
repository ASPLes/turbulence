/*
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
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

/* mutex used to control application top level initialization */
VortexMutex    mod_python_top_init;
/* control if python itself was initialized on this process */
axl_bool       mod_python_py_init = axl_false;

#define APPLICATION_LOAD_FAILED(msg, ...)                         \
        wrn (msg, ##__VA_ARGS__);                                 \
        /* clean for clean start activated */                     \
        CLEAN_START (ctx);                                        \
                                                                  \
        /* get next node */                                       \
        node = axl_node_get_next_called (node, "application");    \
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
axl_bool mod_python_init_app (PyObject * init_function)
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

	py_tbc_ctx = py_turbulence_ctx_create (ctx);

	/* not required to acquire the GIL we are calling from the
	 * thread that called to Py_Initialize */

	/* build parameters */
	args = PyTuple_New (1);
	PyTuple_SetItem (args, 0, py_tbc_ctx);

	/* now call to the function */
	result = PyObject_Call (init_function, args, NULL);

	/* handle exceptions */
	py_vortex_handle_and_clear_exception (NULL);

	/* now parse results to know if we have properly loaded the
	 * python extension */
	if (result) {
		/* now implement other attributes */
		if (! PyArg_Parse (result, "i", &_result)) {
			py_vortex_handle_and_clear_exception (NULL);
			return axl_false;
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
 * continue signaling its proper startup.
 */
axl_bool mod_python_init_applications (const char * workDir, const char * serverName, VortexConnection * conn)
{
	axlNode    * node;
	axlNode    * location;
	PyObject   * module;
	PyObject   * function;
	const char * start_file;

	/* for each application */
	node = axl_doc_get (mod_python_conf, "/mod-python/application");
	while (node) {
		msg ("loading python app [%s]", ATTR_VALUE (node, "name"));

		/* check if node has serverName attribute and compare
		   it with serverName defined (and only if defined) */
		if (serverName && HAS_ATTR (node, "serverName") && ! HAS_ATTR_VALUE (node, "serverName", serverName)) {
			msg2 ("  -> no match for serverName=%s (app serverName=%s)", serverName, ATTR_VALUE (node, "serverName"));
			/* found serverName defined, but application spec states other serverName, skipping this application */
			/* get next node */
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */

		/* now check for applications already loaded */
		if (PTR_TO_INT (axl_node_annotate_get (node, "app-started", axl_false))) {
			msg2 ("  -> app already started");
			/* found application already loaded */
			node = axl_node_get_next_called (node, "application");
			continue;
		} /* end if */
		
		/* find location node */
		location = axl_node_get_child_called (node, "location");
		if (location == NULL) {
			/* log error and continue */
			APPLICATION_LOAD_FAILED ("unable to find location configuration for python app name %s", ATTR_VALUE (node, "name"));
		}

		/* drop a log */
		msg ("importing code found at: %s%s%s", ATTR_VALUE (location, "src"), VORTEX_FILE_SEPARATOR, ATTR_VALUE (location, "start-file"));

		/* add module path to complete import operation */
		if (! mod_python_add_path (ATTR_VALUE (location, "src"))) {
			APPLICATION_LOAD_FAILED ("unable to add sys.path %s to load module %s",
						 ATTR_VALUE (location, "src"), ATTR_VALUE (location, "name"));
		} /* end if */

		/* check import code do not end it .py or .pyc */
		start_file = ATTR_VALUE (location, "start-file");
		if (axl_cmp (start_file + strlen (start_file) - 3, ".py")) {
			wrn ("Start file cannot be ended with .py. This will fail. Try to leave start file without extention");
		}

		/* now load python source */
		module = PyImport_ImportModule (start_file);
		
		if (module == NULL) {
			/* handle possible exception */
			py_vortex_handle_and_clear_exception (NULL);

			APPLICATION_LOAD_FAILED ("Failed to import module %s located at %s", 
						 start_file, 
						 ATTR_VALUE (location, "src"));
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
		if (! mod_python_init_app (function)) {

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

		/* remove path */
		if (! mod_python_remove_first_path ()) {
			APPLICATION_LOAD_FAILED ("Failed to remove path from sys.path after python app initialization",
						 start_file, ATTR_VALUE (location, "app-init"));
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

	/* finish python */
	Py_Finalize ();

	vortex_mutex_destroy (&mod_python_top_init);

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
		/* initialize python */
		Py_Initialize ();

		/* call to initilize threading API and to acquire the lock */
		PyEval_InitThreads();

		/* call to init py-turbulence */
		py_turbulence_init ();

		/* configure exception handler */
		py_vortex_set_exception_handler (mod_python_exception);

		/* signal python initialized */
		mod_python_py_init = axl_true;

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
							   vortex_support_build_filename (turbulence_sysconfdir (), "turbulence", "python", NULL));

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
	} /* end if */

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
	/* PyGILState_STATE   state; */

	serverName = turbulence_ppath_get_server_name (conn);
	workDir    = turbulence_ppath_get_work_dir (ctx, ppath_selected);

	/* lock to check and initialize */
	vortex_mutex_lock (&mod_python_top_init);

	/* initialize python engine and configure it to use with
	   mod-python */
	mod_python_initialize ();

	/*** bridge into python ***/
	/* acquire the GIL */
	/* state = PyGILState_Ensure(); */

	/* check and load mod-python config */
	if (! mod_python_load_config ()) {
		vortex_mutex_unlock (&mod_python_top_init);
		/* release the GIL */
		/* PyGILState_Release(state); */
		PyEval_ReleaseLock ();
		return axl_false;
	}

	/* for each application found, register it and call to
	 * initialize its function */
	if (! mod_python_init_applications (workDir, serverName, conn)) {
		vortex_mutex_unlock (&mod_python_top_init);
		/* release the GIL */
		/* PyGILState_Release(state); */
		PyEval_ReleaseLock ();
		return axl_false;
	}

	/* let other threads to enter inside python engine: this must
	   be the last call: release the GIL */
	/* PyGILState_Release(state); */
	PyEval_ReleaseLock ();

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


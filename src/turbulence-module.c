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

#include <turbulence.h>

/** 
 * \defgroup turbulence_module Turbulence Module: API used to load modules and invoke handlers provided by them.
 */

/** 
 * \addtogroup turbulence_module
 * @{
 */


/* local include */
#include <turbulence-ctx-private.h>

#if defined(AXL_OS_UNIX)
#include <dlfcn.h>
#endif

struct _TurbulenceModule {
	/* module attributes */
	char             * path;
	void             * handle;
	TurbulenceModDef * def;

	/* context that loaded the module */
	TurbulenceCtx    * ctx;

	/* list of profiles provided by this module */
	axlList          * provided_profiles;
};

/** 
 * @internal Starts the turbulence module initializing all internal
 * variables.
 */
void               turbulence_module_init      (TurbulenceCtx * ctx)
{
	/* a list of all modules loaded */
	ctx->registered_modules = axl_list_new (axl_list_always_return_1, 
						(axlDestroyFunc) turbulence_module_free);
	/* init mutex */
	vortex_mutex_create (&ctx->registered_modules_mutex);
	return;
}

/** 
 * @brief Allows to get the module name.
 *
 * @param module The module to get the name from.
 *
 * @return module name or NULL if it fails.
 */
const char       * turbulence_module_name        (TurbulenceModule * module)
{
	axl_return_val_if_fail (module, NULL);
	axl_return_val_if_fail (module->def, NULL);

	/* return stored name */
	return module->def->mod_name;
}

/** 
 * @brief Loads a turbulence module, from the provided path.
 * 
 * @param ctx The context where the module will be opened.
 * @param module The module path to load.
 * 
 * @return A reference to the module loaded or NULL if it fails.
 */
TurbulenceModule * turbulence_module_open (TurbulenceCtx * ctx, const char * module)
{
	TurbulenceModule * result;

	axl_return_val_if_fail (module, NULL);

	/* allocate memory for the result */
	result         = axl_new (TurbulenceModule, 1);
	result->path   = axl_strdup (module);
	result->ctx    = ctx;
#if defined(AXL_OS_UNIX)
	result->handle = dlopen (module, RTLD_LAZY | RTLD_GLOBAL);
#elif defined(AXL_OS_WIN32)
	result->handle = LoadLibraryEx (module, NULL, 0);
	if (result->handle == NULL)
		result->handle = LoadLibraryEx (module, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif

	/* check loaded module */
	if (result->handle == NULL) {
		/* unable to load the module */
#if defined(AXL_OS_UNIX)
		error ("unable to load module (%s): %s", module, dlerror ());
#elif defined(AXL_OS_WIN32)
		error ("unable to load module (%s), error code: %d", module, GetLastError ());
#endif

		/* free the result and return */
		turbulence_module_free (result);
		return NULL;
	} /* end if */

	/* find the module */
#if defined(AXL_OS_UNIX)
	result->def = (TurbulenceModDef *) dlsym (result->handle, "module_def");
#elif defined(AXL_OS_WIN32)
	result->def = (TurbulenceModDef *) GetProcAddress (result->handle, "module_def");
#endif
	if (result->def == NULL) {
		/* unable to find module def */
#if defined(AXL_OS_UNIX)
		error ("unable to find 'module_def' symbol, it seems it isn't a turbulence module: %s", dlerror ());
#elif defined(AXL_OS_WIN32)
		error ("unable to find 'module_def' symbol, it seems it isn't a turbulence module: error code %d", GetLastError ());
#endif
		
		/* free the result and return */
		turbulence_module_free (result);
		return NULL;
	} /* end if */
	
	msg ("module found: [%s]", result->def->mod_name);
	
	return result;
}

/** 
 * @brief Allows to close the provided module, unloading from memory
 * and removing all registration references. The function do not
 * perform any module space notification. The function makes use of
 * the unload function implemented by modules (if defined).
 *
 * @param ctx The context where the module must be removed.
 * @param module The module name to be unloaded.
 */
void               turbulence_module_unload       (TurbulenceCtx * ctx,
						   const char    * module)
{
	int                iterator;
	TurbulenceModule * mod_added;

	/* check values received */
	if (ctx == NULL || module == NULL)
		return;

	/* register the module */
	vortex_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module)) {
			/* call to unload module */
			if (mod_added->def->unload != NULL)
				mod_added->def->unload (ctx);

			/* remove from registered modules */
			axl_list_remove_at (ctx->registered_modules, iterator);

			/* terminate it */
			vortex_mutex_unlock (&ctx->registered_modules_mutex);
			return;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	/* no module with the same name was found */
	return;
}

/** 
 * @brief Allows to get the init function for the module reference
 * provided.
 * 
 * @param module The module that is being requested to return the init
 * function.
 * 
 * @return A reference to the init function or NULL if the function
 * doesn't have a init function defined (which is possible and allowed).
 */
ModInitFunc        turbulence_module_get_init  (TurbulenceModule * module)
{
	/* check the reference received */
	if (module == NULL)
		return NULL;

	/* return the reference */
	return module->def->init;
}

/** 
 * @brief Allows to get the close function for the module reference
 * provided.
 * 
 * @param module The module that is being requested to return the
 * close function.
 * 
 * @return A refernce to the close function or NULL if the function
 * doesn't have a close function defined (which is possible and
 * allowed).
 */
ModCloseFunc       turbulence_module_get_close (TurbulenceModule * module)
{
	/* check the reference received */
	if (module == NULL)
		return NULL;

	/* return the reference */
	return module->def->close;
}

/** 
 * @brief Allows to check if there is another module already
 * registered with the same name.
 */
axl_bool           turbulence_module_exists      (TurbulenceModule * module)
{
	TurbulenceCtx    * ctx;
	int                iterator;
	TurbulenceModule * mod_added;

	/* check values received */
	v_return_val_if_fail (module, axl_false);

	/* get context reference */
	ctx = module->ctx;

	/* register the module */
	vortex_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module->def->mod_name)) {
			vortex_mutex_unlock (&ctx->registered_modules_mutex);
			return axl_true;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	/* no module with the same name was found */
	return axl_false;
}

/** 
 * @brief Allows to register the module loaded to allow future access
 * to it.
 * 
 * @param module The module being registered.
 *
 * @return axl_false in the case module was not registered, otherwise
 * axl_true when the module is registered.
 */
axl_bool             turbulence_module_register  (TurbulenceModule * module)
{
	TurbulenceCtx    * ctx;
	int                iterator;
	TurbulenceModule * mod_added;

	/* check values received */
	v_return_val_if_fail (module, axl_false);

	/* get context reference */
	ctx = module->ctx;

	/* register the module */
	vortex_mutex_lock (&ctx->registered_modules_mutex);

	/* check first that the module is not already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get module in position */
		mod_added = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check mod name */
		if (axl_cmp (mod_added->def->mod_name, module->def->mod_name)) {
			wrn ("skipping module found: %s, already found a module registered with the same name, at path: %s",
			     module->def->mod_name, mod_added->path);
			vortex_mutex_unlock (&ctx->registered_modules_mutex);
			return axl_false;
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	axl_list_add (ctx->registered_modules, module);
	msg ("Registered modules (%d, %p)", axl_list_length (ctx->registered_modules), ctx->registered_modules);
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	return axl_true;
}

/** 
 * @brief Unregister the module provided from the list of modules
 * loaded. The function do not close the module (\ref
 * turbulence_module_free). This is required to be done by the
 * caller.
 * 
 * @param module The module to unregister.
 */
void               turbulence_module_unregister  (TurbulenceModule * module)
{
	TurbulenceCtx    * ctx;

	/* check values received */
	v_return_if_fail (module);

	/* get a reference to the context */
	ctx = module->ctx;

	/* register the module */
	vortex_mutex_lock (&ctx->registered_modules_mutex);
	axl_list_unlink (ctx->registered_modules, module);
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	return;
}

/** 
 * @brief High level function that opens the module located at the
 * provided location, checking if the module was registered, calling
 * to init it, and then registering it if the init did not fail.
 *
 * @param ctx The context where the load operation will take place.
 * @param location The location where the module is found.
 *
 * @return A reference to the module loaded (\ref TurbulenceModule) or NULL if it fails.
 */
TurbulenceModule           * turbulence_module_open_and_register (TurbulenceCtx * ctx, const char * location)
{
	TurbulenceModule * module;
	ModInitFunc        init;

	if (ctx == NULL || location == NULL)
		return NULL;

	/* load the module man!!! */
	module = turbulence_module_open (ctx, location);
	if (module == NULL) {
		wrn ("unable to open module: %s", location);
		return NULL;
	} /* end if */

	/* check module name */
	if (! turbulence_run_check_no_load_module (ctx, turbulence_module_name (module))) {
		wrn ("module %s skipped by plugin name", turbulence_module_name (module));
		turbulence_module_free (module);
		return NULL; /* skipping a module *is* a failure, that is, the module was not loaded. */
	}

	/* check if module exists */
	if (turbulence_module_exists (module)) {
		wrn ("unable to load module: %s, another module is already loaded with the same name",
		     turbulence_module_name (module));

		/* close the module */
		turbulence_module_free (module);
		return NULL;
	}

	/* init the module */
	init = turbulence_module_get_init (module);
		
	/* check init */
	if (! init (ctx)) {
		wrn ("init module: %s have failed, skiping", location);
		CLEAN_START(ctx);
		
		/* close the module */
		turbulence_module_free (module);
		
		return NULL;
		
	} else {
		msg ("init ok, registering module: %s", location);
	}

	/* register the module to be loaded */
	turbulence_module_register (module);

	/* now the module is registered, publish this is done */
	turbulence_mediator_push_event (ctx, "turbulence", "module-registered", 
					/* publish name added */
					(axlPointer) turbulence_module_name (module), NULL, NULL, NULL);
	return module;
}

/** 
 * @brief Closes and free the module reference.
 * 
 * @param module The module reference to free and close.
 */
void               turbulence_module_free (TurbulenceModule * module)
{
	/* check values received */
	v_return_if_fail (module);

	axl_free (module->path);
	/* call to unload the module */
	if (module->handle) {
#if defined(AXL_OS_UNIX)
		dlclose (module->handle);
#elif defined(AXL_OS_WIN32)
		FreeLibrary (module->handle);
#endif
	}
	axl_free (module);
	
	
	return;
}

/** 
 * @brief Allows to do a handler notification on all registered
 * modules.
 * @param ctx The context where the notification will take place.
 * @param handler The handler to be called.
 *
 * @param data Optional data to be passed to the handler. This pointer
 * is handler especific.
 *
 * @param data2 Second optional data to be passed to the handler. This
 * pointer is handler especific.
 *
 * @param data3 Third optional data to be passed to the handler. This
 * pointer is handler especific.
 *
 * @return Returns axl_true if all handlers executed also returned
 * axl_true. Those handler that have no return value will cause the
 * function to always return axl_true. 
 */
axl_bool           turbulence_module_notify      (TurbulenceCtx         * ctx, 
						  TurbulenceModHandler    handler,
						  axlPointer              data,
						  axlPointer              data2,
						  axlPointer              data3)
{
	/* get turbulence context */
	TurbulenceModule * module;
	int                iterator = 0;

	vortex_mutex_lock (&ctx->registered_modules_mutex);
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get the module */
		module = axl_list_get_nth (ctx->registered_modules, iterator);

		/* check reference */
		if (module == NULL || module->def == NULL) {
			iterator++;
			continue;
		} /* end if */
			
		switch (handler) {
		case TBC_CLOSE_HANDLER:
			/* notify if defined reconf function */
			if (module->def->close != NULL) {
				msg ("closing module: %s (%s)", module->def->mod_name, module->path);
				vortex_mutex_unlock (&ctx->registered_modules_mutex);
				module->def->close (ctx);
				vortex_mutex_lock (&ctx->registered_modules_mutex);
			}
			break;
		case TBC_RELOAD_HANDLER:
			/* notify if defined reconf function */
			if (module->def->reconf != NULL) {
				msg ("reloading module: %s (%s)", module->def->mod_name, module->path);
				vortex_mutex_unlock (&ctx->registered_modules_mutex);
				module->def->reconf (ctx);
				vortex_mutex_lock (&ctx->registered_modules_mutex);
			}
			break;
		case TBC_INIT_HANDLER:
			/* notify if defined reconf function */
			if (module->def->init != NULL) {
				msg ("initializing module: %s (%s)", module->def->mod_name, module->path);
				vortex_mutex_unlock (&ctx->registered_modules_mutex);
				if (! module->def->init (ctx)) {
					/* init failed */
					wrn ("failed to initialized module: %s, it returned initialization failure", module->def->mod_name);
					return axl_false;
				}
				vortex_mutex_lock (&ctx->registered_modules_mutex);
					
			}
			break;
		case TBC_PPATH_SELECTED_HANDLER:
			/* notify if defined reconf function */
			if (module->def->ppath_selected != NULL) {
				msg ("notifying profile path selected on module: %s (%s)", module->def->mod_name, module->path);
				vortex_mutex_unlock (&ctx->registered_modules_mutex);
				if (! module->def->ppath_selected (ctx, data, data2))  {
					/* init failed */
					wrn ("profile path selection for module: %s returned failure", module->def->mod_name);
					return axl_false;
				} /* end if */
				vortex_mutex_lock (&ctx->registered_modules_mutex);
			}
			break;
		}

		/* next iterator */
		iterator++;

	} /* end if */
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	/* reached this point always return TRUE dude!! */
	return axl_true;
}


/** 
 * @brief Allows to notify all modules loaded, that implements the
 * \ref ModReconfFunc, to reload its configuration data.
 */
void               turbulence_module_notify_reload_conf (TurbulenceCtx * ctx)
{
	turbulence_module_notify (ctx, TBC_RELOAD_HANDLER, NULL, NULL, NULL);

	return;
}

/** 
 * @brief Send a module close notification to all modules registered
 * without unloading module code.
 */
void               turbulence_module_notify_close (TurbulenceCtx * ctx)
{
	turbulence_module_notify (ctx, TBC_CLOSE_HANDLER, NULL, NULL, NULL);

	return;
}

/** 
 * @brief Cleans the module, releasing all resources and unloading all
 * modules.
 */
void               turbulence_module_cleanup   (TurbulenceCtx * ctx)
{
	/* do not operate if a null value is received */
	if (ctx == NULL)
		return;

	/* release the list and all modules */
	msg ("Cleaning up turbulence %d modules (ctx: %p)..", axl_list_length (ctx->registered_modules), ctx);
	axl_list_free (ctx->registered_modules);
	ctx->registered_modules = NULL;
	vortex_mutex_destroy (&ctx->registered_modules_mutex);

	return;
}



/** 
 * @}
 */

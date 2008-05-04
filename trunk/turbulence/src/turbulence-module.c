/*  Turbulence:  BEEP application server
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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

#include <turbulence.h>

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
 * @brief Loads a turbulence module, from the provided path.
 * 
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
	result->handle = dlopen (module, RTLD_LAZY);
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
 * @brief Allows to register the module loaded to allow future access
 * to it.
 * 
 * @param module The module being registered.
 */
void               turbulence_module_register  (TurbulenceModule * module)
{
	TurbulenceCtx * ctx;

	/* check values received */
	v_return_if_fail (module);

	/* get context reference */
	ctx = module->ctx;

	/* register the module */
	vortex_mutex_lock (&ctx->registered_modules_mutex);
	axl_list_add (ctx->registered_modules, module);
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

	return;
}

/** 
 * @brief Unregister the module provided from the list of modules
 * loaded. The function do not close the module (\ref
 * turbulence_module_close). This is required to be done by the
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
 * @brief Closes and free the module reference.
 * 
 * @param module The module reference to free and close.
 */
void               turbulence_module_free (TurbulenceModule * module)
{
	TurbulenceCtx    * ctx;

	/* check values received */
	v_return_if_fail (module);

	/* get a reference to the context */
	ctx = module->ctx;

	/* call to close the module */
	if (module->def != NULL && module->def->close != NULL) {
		msg ("closing module: %s", module->def->mod_name);
		module->def->close (ctx);
	}

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
 * @brief Allows to notify all modules loaded, that implements the
 * \ref ModReconfFunc, to reload its configuration data.
 */
void               turbulence_module_notify_reload_conf (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	TurbulenceModule * module;

	int iterator = 0;
	vortex_mutex_lock (&ctx->registered_modules_mutex);
	while (iterator < axl_list_length (ctx->registered_modules)) {
		/* get the module */
		module = axl_list_get_nth (ctx->registered_modules, iterator);

		/* notify if defined reconf function */
		if (module->def->reconf != NULL) {
			/* call to reconfigured */
			module->def->reconf (ctx);
		}

		/* next iterator */
		iterator++;

	} /* end if */
	vortex_mutex_unlock (&ctx->registered_modules_mutex);

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
	axl_list_free (ctx->registered_modules);
	ctx->registered_modules = NULL;
	vortex_mutex_destroy (&ctx->registered_modules_mutex);

	return;
}




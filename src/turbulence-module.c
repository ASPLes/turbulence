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
#include <dlfcn.h>

struct _TurbulenceModule {
	char             * path;
	void             * handle;
	TurbulenceModDef * def;
};

/** 
 * @internal A list of modules registered.
 */
axlList * __registered_modules = NULL;

/** 
 * @brief Starts the turbulence module initializing all internal
 * variables.
 */
void               turbulence_module_init      ()
{
	/* a list of all modules loaded */
	__registered_modules = axl_list_new (axl_list_always_return_1, 
					     (axlDestroyFunc) turbulence_module_free);
	return;
}

/** 
 * @brief Loads a turbulence module, from the provided path.
 * 
 * @param module The module path to load.
 * 
 * @return A reference to the module loaded or NULL if it fails.
 */
TurbulenceModule * turbulence_module_open (const char * module)
{
	TurbulenceModule * result;

	axl_return_val_if_fail (module, NULL);

	/* allocate memory for the result */
	result         = axl_new (TurbulenceModule, 1);
	result->path   = axl_strdup (module);
	result->handle = dlopen (module, RTLD_LAZY);

	/* check loaded module */
	if (result->handle == NULL) {
		/* unable to load the module */
		error ("unable to load module (%s): %s", dlerror ());

		/* free the result and return */
		turbulence_module_free (result);
		return NULL;
	} /* end if */

	/* find the module */
	result->def = dlsym (result->handle, "module_def");
	if (result->def == NULL) {
		/* unable to find module def */
		error ("unable to find 'module_def' symbol, it seems it isn't a turbulence module: %s", dlerror ());
		
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
	/* register the module */
	axl_list_add (__registered_modules, module);

	return;
}

/** 
 * @brief Closes and free the module reference.
 * 
 * @param module The module reference to free and close.
 */
void               turbulence_module_free (TurbulenceModule * module)
{
	/* check the module reference */
	if (module == NULL)
		return;

	/* call to close the module */
	if (module->def->close != NULL) {
		msg ("closing module: %s", module->def->mod_name);
		module->def->close ();
	}

	axl_free (module->path);
	/* call to unload the module */
	if (module->handle)
		dlclose (module->handle);
	axl_free (module);
	
	
	return;
}


/** 
 * @brief Allows to notify all modules loaded, that implements the
 * \ref ModReconfFunc, to reload its configuration data.
 */
void               turbulence_module_notify_reload_conf ()
{
	TurbulenceModule * module;

	int iterator = 0;
	while (iterator < axl_list_length (__registered_modules)) {
		/* get the module */
		module = axl_list_get_nth (__registered_modules, iterator);

		/* notify if defined reconf function */
		if (module->def->reconf != NULL) {
			/* call to reconfigured */
			module->def->reconf ();
		}

		/* next iterator */
		iterator++;

	} /* end if */
}

/** 
 * @brief Cleans the module, releasing all resources and unloading all
 * modules.
 */
void               turbulence_module_cleanup   ()
{
	/* release the list and all modules */
	axl_list_free (__registered_modules);
}




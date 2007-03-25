/*
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
#include <dlfcn.h>

struct _TurbulenceModule {
	char * path;
	void * handle;
};

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
	TurbulenceModDef * def;

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
	def = dlsym (result->handle, "module_def");
	if (def == NULL) {
		/* unable to find module def */
		error ("unable to find 'module_def' symbol, it seems it isn't a turbulence module: %s", dlerror ());
		
		/* free the result and return */
		turbulence_module_free (result);
		return NULL;
	} /* end if */
	
	msg ("module found: [%s]", def->mod_name);
	
	return result;
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

	axl_free (module->path);
	/* call to unload the module */
	if (module->handle)
		dlclose (module->handle);
	axl_free (module);
	
	
	return;
}



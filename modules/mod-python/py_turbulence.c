/** 
 *  PyTurbulence: Python bindings for Turbulence API 
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
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
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

/* include base */
#include <py_turbulence.h>

/** 
 * @brief Allows to init turbulence Python api. This is general done
 * by the python interepreter but it is required to do it manually if
 * used from mod-python directly.
 */
void            py_turbulence_init (void)
{
	/* call to initialize */
	initlibpy_turbulence ();

	return;
}

PyMODINIT_FUNC  initlibpy_turbulence (void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	/* PyEval_InitThreads(); */

	/* register vortex module */
	module = Py_InitModule3 ("turbulence", NULL, 
				 "Turbulence API python bindings");
	if (module == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to create turbulence module");
		return;
	} /* end if */

	/* call to init other modules */
	init_turbulence_ctx (module);

	py_vortex_log (PY_VORTEX_DEBUG, "PyTurbulence module started");

	return;
}



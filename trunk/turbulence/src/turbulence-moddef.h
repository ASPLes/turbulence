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
#ifndef __TURBULENCE_MODDEF_H__
#define __TURBULENCE_MODDEF_H__

/** 
 * \defgroup turbulence_moddef Turbulence Module Def: Type definitions for modules
 */

/**
 * \addtogroup turbulence_moddef
 * @{
 */

/** 
 * @brief Public definition for the init function that must implement
 * a turbulence module.
 *
 * The module must return axl_true to signal the modules was
 * initialized and must be registered as properly loaded. This handler
 * is called when the module is loaded and after a fork operation
 * (child process created) when turbulence signals the module to
 * reinitialize some structures that may be affected by the fork
 * operation (like mutexes, threads..).
 *
 * @param ctx The turbulence context where the init operation is
 * taking place.
 * 
 * @return axl_true if the module is usable or axl_false if
 * not. Returning axl_false caused the module to be not loaded.
 */
typedef axl_bool  (*ModInitFunc)  (TurbulenceCtx * ctx);

/** 
 * @brief Public definition for the close function that must implement
 * all operations required to unload and terminate a module.
 * 
 * The function doesn't receive and return any data.
 *
 * @param ctx The turbulence context where the close operation is
 * taking place.
 */
typedef void (*ModCloseFunc) (TurbulenceCtx * ctx);

/** 
 * @brief Public definition for the reconfiguration function that must
 * be implemented to receive notification if the turbulence server
 * configuration is reloaded (either because a signal was received or
 * because some module has called \ref turbulence_reload_config).
 *
 * @param ctx The turbulence context where the reconf operation is
 * taking place.
 */
typedef void (*ModReconfFunc) (TurbulenceCtx * ctx);

/** 
 * @brief Allows to notify the module that a profile path was selected
 * for a connection.
 *
 * @param ctx The turbulence context where the profile path is being selected.
 * @param ppath_selected The profile path selected.
 * @param conn The connection where the profile path was selected.
 *
 * @return Like \ref ModInitFunc "init" function, if this handler
 * returns axl_false the connection is closed.
 */
typedef axl_bool (*ModPPathSelected) (TurbulenceCtx * ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn);


/** 
 * @brief Public definition for the main entry point for all modules
 * developed for turbulence.
 * 
 * See <a class="el" href="http://www.aspl.es/turbulence/extending.html">how to create Turbulence modules</a>.
 */
typedef struct _TurbulenceModDef {
	/** 
	 * @brief The module name. This name is used by turbulence to
	 * refer to the module.
	 */
	char         * mod_name;

	/** 
	 * @brief The module long description.
	 */
	char         * mod_description;

	/** 
	 * @brief A reference to the init function associated to the
	 * module.
	 */
	ModInitFunc    init;

	/** 
	 * @brief A reference to the close function associated to the
	 * module.
	 */
	ModCloseFunc   close;
	
	/** 
	 * @brief A reference to the reconf function that must be
	 * implemented to get notifications about turbulence server
	 * configuration changes.
	 */
	ModReconfFunc  reconf;

	/** 
	 * @brief A reference to the unload function that must
	 * implement all unload code required in the case turbulence
	 * function ask the module to stop its function for a
	 * particular process (which means the module may be working
	 * in another module). 
	 * 
	 * The difference between close and reload is that close can
	 * consider alls its actions as definitive because turbulence
	 * main process is stopping.
	 *
	 * However, unload function is only called in the context of
	 * child process created by turbulence to isolate some
	 * requests (modules and profiles) to be handled by a low
	 * permissions user.
	 */
	ModCloseFunc  unload;

	/** 
	 * @brief A reference to the profile path selected handler.
	 */
	ModPPathSelected ppath_selected;
} TurbulenceModDef;

/** 
 * @brief Allows to prepare de module with the turbulence context (and
 * the vortex context associated).
 *
 * This macro must be called inside the module init, before any
 * operation is done. 
 * 
 * @param _ctx The context received by the module at the init functio.
 */
#define TBC_MOD_PREPARE(_ctx) do{ctx = _ctx;}while(0)

#endif

/* @} */

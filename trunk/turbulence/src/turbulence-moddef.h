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
 * The init function doesn't receive any thing, but it must return
 * true to signal that the modules was initialized and must be
 * registered.
 * 
 * @return true if the module is usable or false if not.
 */
typedef bool (*ModInitFunc)  (TurbulenceCtx * ctx);

/** 
 * @brief Public definition for the close function that must implement
 * all operations required to unload module.
 * 
 * The function doesn't receive and return any data.
 */
typedef void (*ModCloseFunc) (TurbulenceCtx * ctx);

/** 
 * @brief Public definition for the reconfiguration function that must
 * be implemented to receive notification if the turbulence server
 * configuration is reloaded.
 */
typedef void (*ModReconfFunc) (TurbulenceCtx * ctx);


/**
 * @brief Public definition for the main entry point for all modules
 * developed for turbulence.
 * 
 * See <a class="el" href="http://www.turbulence.ws/extending.html">how to create Turbulence modules</a>.
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
	 * @brief A reference to the init function associated to the module.
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
} TurbulenceModDef;

/** 
 * @brief Allows to prepare de module with the turbulence context (and
 * the vortex context associated).
 *
 * This macro must be called inside the module init, before any
 * operation is done. 
 * 
 * @param ctx The context received by the module at the init functio.
 */
#define TBC_MOD_PREPARE(_ctx) do{ctx = _ctx;}while(0)

/** 
 * @brief Allows to get the vortex context associated to the
 * turbulence context provided.
 * 
 * @param _ctx The turbulence context which is required to return the
 * vortex context associated.
 * 
 * @return A reference to the vortex context associated.
 */
#define TBC_VORTEX_CTX(_ctx) (turbulence_ctx_get_vortex_ctx (_ctx))

#endif

/* @} */

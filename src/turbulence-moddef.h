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
#ifndef __TURBULENCE_MODDEF_H__
#define __TURBULENCE_MODDEF_H__

/** 
 * \defgroup turbulence_moddef Turbulence Module Def: Type and handler definitions for turbulence modules
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
 * properly initialized so turbulence can register it as usable.
 *
 * This handler is called only once at turbulence startup and when a
 * child process is created (separate="yes" directive inside profile
 * path configuration, see \ref turbulence_execution_model and \ref profile_path_configuration). 
 *
 * To know if current init call is for a child process you can use:
 *
 * \code
 * if (turbulence_ctx_is_child (ctx) {
 *      // reinitialize 
 * } else {
 *      // normal init
 * }
 * \endcode
 *
 * Keep in mind child creation is implemented using fork() call
 * (without further exec()). This means you may require reinitialize
 * some structures (like mutex). 
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
 * @brief A reference to the unload method that must implement all
 * unload code required in the case turbulence ask the module
 * to stop its function.
 * 
 * Unload function is only called in the context of child process
 * created by turbulence to isolate some requests (modules and
 * profiles) to be handled by a low permissions user.
 *
 * @param ctx The turbulence context where the close operation is
 * taking place.
 */
typedef void (*ModUnloadFunc) (TurbulenceCtx * ctx);

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
 * The ppath_selected function is used by turbulence to signal modules
 * that a connection was finally configured under the provided profile
 * path. This is important because a profile path defines how the
 * connection will be limited and configured to accept profiles,
 * configuring process permission and so on.
 *
 * It is also useful because at the time a profile path is selected,
 * serverName name is available, allowing the module to take especial
 * actions.
 *
 * This module function is called just after a profile path is
 * selected for a connection, on first channel request received,
 * (according to serverName, src connection, etc). In the case the
 * profile path request a child process to handle the connection, the
 * module function is called after creating the child process and
 * after the child process implements optional chroot and optional
 * user:group change.
 *
 * In the case the profile path do not request to create a child
 * process, the handler is called on first channel request received on
 * a connection.
 *
 * Note that, unlike \ref ModInitFunc "init", this method is called
 * for each connection received for which a profile path is selected. 
 *
 * <b>Security: Important note about serverName</b>
 *
 * When a profile path is selected it is done according to preliminary
 * data provided by remote peer. In this context the module programmer
 * must see the serverName as a requested value but still not the
 * final serverName accepted. 
 *
 * The serverName will only be definitive only if all profile path
 * handlers accept it and a channel with that serverName is created.
 *
 * In this context, to get the serverName requested by remote peer you can use:
 * \code
 * serverName = turbulence_ppath_get_server_name (conn);
 * \endcode
 *
 *
 *
 * @param ctx The turbulence context where the profile path is being
 * selected.
 *
 * @param ppath_selected The profile path selected. Reference to the
 * object representing the profile path selected. See \ref
 * turbulence_ppath.
 *
 * @param conn The connection where the profile path was selected. The
 * VortexConnection object that was configured with the provided
 * profile path.
 *
 * @return Like \ref ModInitFunc "init" function, if this handler
 * returns axl_false the connection is closed. Keep in mind returning
 * axl_false will terminate current process.
 */
typedef axl_bool (*ModPPathSelected) (TurbulenceCtx * ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn);


/** 
 * @brief Public definition for the main entry point for all modules
 * developed for turbulence. 
 *
 * This structure contains pointers to all functions that may
 * implement a turbulence module.
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
	 * @brief A reference to the reconf function associated to the
	 * module.
	 */
	ModReconfFunc  reconf;

	/** 
	 * @brief A reference to the unload function associated to the
	 * module.
	 */
	ModUnloadFunc  unload;

	/** 
	 * @brief A reference to the profile path selected function
	 * associated to the module.
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

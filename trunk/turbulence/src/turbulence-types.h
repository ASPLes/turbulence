/*  Turbulence:  BEEP application server
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
#ifndef __TURBULENCE_TYPES_H__
#define __TURBULENCE_TYPES_H__

/** 
 * \defgroup turbulence_types Turbulence types: types used/exposed by Turbulence API
 */

/** 
 * \addtogroup turbulence_types
 * @{
 */

/** 
 * @internal Type used by profile path module.
 */
typedef struct _TurbulencePPathItem TurbulencePPathItem;

/** 
 * @internal Type that represents all turbulence profile paths
 */
typedef struct _TurbulencePPath TurbulencePPath;

/** 
 * @brief Type definition that represents a singl profile path configuration.
 */
typedef struct _TurbulencePPathDef TurbulencePPathDef;

/** 
 * @brief Type representing a loop watching a set of files. See \ref turbulence_loop.
 */
typedef struct _TurbulenceLoop TurbulenceLoop;

/** 
 * @brief Type that represents a turbulence module.
 */
typedef struct _TurbulenceModule TurbulenceModule;

/** 
 * @brief Type representing a child process created. Abstraction used
 * to store a set of data used around the child.
 */
typedef struct _TurbulenceChild  TurbulenceChild;

/** 
 * @brief Set of handlers that are supported by modules. This handler
 * descriptors are used by some functions to notify which handlers to
 * call: \ref turbulence_module_notify.
 */
typedef enum {
	/** 
	 * @brief Module reload handler 
	 */
	TBC_RELOAD_HANDLER = 1,
	/** 
	 * @brief Module close handler 
	 */
	TBC_CLOSE_HANDLER  = 2,
	/** 
	 * @brief Module init handler 
	 */
	TBC_INIT_HANDLER   = 3,
	/** 
	 * @brief Module profile path selected handler.
	 */ 
	TBC_PPATH_SELECTED_HANDLER = 4
} TurbulenceModHandler;

#endif

/** 
 * @}
 */

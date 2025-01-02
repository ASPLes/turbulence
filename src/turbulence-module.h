/*  Turbulence BEEP application server
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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
#ifndef __TURBULENCE_MODULE_H__
#define __TURBULENCE_MODULE_H__

#include <turbulence.h>

void               turbulence_module_init        (TurbulenceCtx * ctx);

TurbulenceModule * turbulence_module_open        (TurbulenceCtx * ctx, 
						  const char    * module);

void               turbulence_module_unload      (TurbulenceCtx * ctx,
						  const char    * module);

const char       * turbulence_module_name        (TurbulenceModule * module);

ModInitFunc        turbulence_module_get_init    (TurbulenceModule * module);

ModCloseFunc       turbulence_module_get_close   (TurbulenceModule * module);

axl_bool           turbulence_module_exists      (TurbulenceModule * module);

axl_bool           turbulence_module_register    (TurbulenceModule * module);

void               turbulence_module_unregister  (TurbulenceModule * module);

TurbulenceModule * turbulence_module_open_and_register (TurbulenceCtx * ctx, 
							const char * location);

void               turbulence_module_skip_unmap  (TurbulenceCtx * ctx, 
						  const char * mod_name);

void               turbulence_module_free        (TurbulenceModule  * module);

axl_bool           turbulence_module_notify      (TurbulenceCtx         * ctx, 
						  TurbulenceModHandler    handler,
						  axlPointer              data,
						  axlPointer              data2,
						  axlPointer              data3);

void               turbulence_module_notify_reload_conf (TurbulenceCtx * ctx);

void               turbulence_module_notify_close (TurbulenceCtx * ctx);

void               turbulence_module_set_no_unmap_modules (axl_bool status);

void               turbulence_module_cleanup      (TurbulenceCtx * ctx);

#endif

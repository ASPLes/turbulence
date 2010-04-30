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
#ifndef __TURBULENCE_CONFIG_H__
#define __TURBULENCE_CONFIG_H__

#include <turbulence.h>

axl_bool        turbulence_config_load     (TurbulenceCtx * ctx, 
					    const char    * config);

void            turbulence_config_cleanup  (TurbulenceCtx * ctx);

axlDoc        * turbulence_config_get      (TurbulenceCtx * ctx);

axl_bool        turbulence_config_is_attr_positive (TurbulenceCtx * ctx,
						    axlNode       * node,
						    const char    * attr_name);

axl_bool        turbulence_config_is_attr_negative (TurbulenceCtx * ctx,
						    axlNode       * node,
						    const char    * attr_name);

#endif

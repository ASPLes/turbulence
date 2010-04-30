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
#ifndef __TURBULENCE_CTX_H__
#define __TURBULENCE_CTX_H__

#include <turbulence.h>

/**
 * \addtogroup turbulence_ctx
 * @{
 */

/** 
 * @brief Turbulence execution context.
 */
typedef struct _TurbulenceCtx TurbulenceCtx;

TurbulenceCtx * turbulence_ctx_new            (void);

void            turbulence_ctx_reinit         (TurbulenceCtx * ctx);

void            turbulence_ctx_set_vortex_ctx (TurbulenceCtx * ctx, 
					       VortexCtx     * vortex_ctx);

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

VortexCtx     * turbulence_ctx_get_vortex_ctx (TurbulenceCtx * ctx);

void            turbulence_ctx_set_data       (TurbulenceCtx * ctx,
					       const char    * key,
					       axlPointer      data);

void            turbulence_ctx_set_data_full  (TurbulenceCtx * ctx,
					       const char    * key,
					       axlPointer      data,
					       axlDestroyFunc  key_destroy,
					       axlDestroyFunc  value_destroy);

axlPointer      turbulence_ctx_get_data       (TurbulenceCtx * ctx,
					       const char    * key);

void            turbulence_ctx_free           (TurbulenceCtx * ctx);

/* @} */

#endif

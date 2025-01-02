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
#ifndef __TURBULENCE_EXPR_H__
#define __TURBULENCE_EXPR_H__

#include <turbulence.h>

/** 
 * \addtogroup turbulence_expr
 * @{
 */

/** 
 * @brief Regular expression type definition provided by the \ref
 * turbulence_expr module. This type is used to represent an regular
 * expression that can be used to match strings.
 */ 
typedef struct _TurbulenceExpr TurbulenceExpr;

TurbulenceExpr * turbulence_expr_compile (TurbulenceCtx * ctx, 
					  const char    * expression, 
					  const char    * error_msg);

axl_bool         turbulence_expr_match   (TurbulenceExpr * expr, 
					  const char     * subject);

/** 
 * @brief Alias definition associated to turbulence_expr_get_expression.
 * @param expr The TurbulenceExpr where the associated string expression is being queried.
 * @return A reference to the string expression.
 */
#define __TBC_EXP_STR__(expr) turbulence_expr_get_expression(expr)
const char     * turbulence_expr_get_expression (TurbulenceExpr * expr);

void             turbulence_expr_free    (TurbulenceExpr * expr);

#endif /* __TURBULENCE_EXPR_H__ */

/** 
 * @}
 */

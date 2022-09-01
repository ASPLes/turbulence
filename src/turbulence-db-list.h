/*  Turbulence BEEP application server
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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
#ifndef __TURBULENCE_DB_LIST_H__
#define __TURBULENCE_DB_LIST_H__

#include <turbulence.h>

/**
 * \addtogroup turbulence_db_list
 * @{
 */

/** 
 * @brief Type definition for an opened turbulence db list: an
 * abstraction that provides easy access to an item list automatically
 * stored.
 */
typedef struct _TurbulenceDbList TurbulenceDbList;

TurbulenceDbList * turbulence_db_list_open   (TurbulenceCtx   * ctx,
					      axlError       ** error, 
					      const char      * token, 
					      ...);

axl_bool           turbulence_db_list_exists (TurbulenceDbList * list,
					      const char       * value);

axl_bool           turbulence_db_list_add    (TurbulenceDbList * list,
					      const char       * value);

axl_bool           turbulence_db_list_remove (TurbulenceDbList * list,
					      const char       * value);

axl_bool           turbulence_db_list_remove_by_func (TurbulenceDbList           * list,
						      TurbulenceDbListRemoveFunc   func,
						      axlPointer                   user_data);

axl_bool           turbulence_db_list_edit   (TurbulenceDbList * list,
					      const char       * oldValue,
					      const char       * newValue);

axlList          * turbulence_db_list_get            (TurbulenceDbList * list);

axl_bool           turbulence_db_list_close          (TurbulenceDbList * list);

axl_bool           turbulence_db_list_unload         (TurbulenceDbList * list);

axl_bool           turbulence_db_list_close_internal (TurbulenceDbList * list,
						      axl_bool           dump_on_close);


axl_bool           turbulence_db_list_reload         (TurbulenceDbList * list);

axl_bool           turbulence_db_list_flush          (TurbulenceDbList * list);

axl_bool           turbulence_db_list_count          (TurbulenceDbList * list);


/* internal services, used by turbulence engine, never by user
 * application code */
axl_bool           turbulence_db_list_init           (TurbulenceCtx * ctx);

void               turbulence_db_list_cleanup        (TurbulenceCtx * ctx);

axl_bool           turbulence_db_list_reload_module  (void);

axl_bool           turbulence_db_list_equal (axlPointer a, axlPointer b);

#endif

/* @} */

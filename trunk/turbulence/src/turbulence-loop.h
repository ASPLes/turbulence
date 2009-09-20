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
#ifndef __TURBULENCE_LOOP_H__
#define __TURBULENCE_LOOP_H__

#include <turbulence.h>

typedef struct _TurbulenceLoop TurbulenceLoop;

/** 
 * @brief Handler definition used by turbulence_loop_set_read_handler
 * to notify that the descriptor is ready to be read (either because
 * it has data or because it was closed).
 *
 * @param loop The loop wher the notification was found.
 * @param ctx The Turbulence context where the loop is running.
 * @param descriptor The descriptor that is ready to be read.
 * @param ptr User defined pointer defined at \ref turbulence_loop_set_read_handler and passed to this handler.
 * @param ptr2 User defined pointer defined at \ref turbulence_loop_set_read_handler and passed to this handler.
 *
 * @return The function return axl_true in the case the read operation
 * was completed without problem. Otherwise axl_false is returned
 * indicating that the turbulence loop engine should close the
 * descriptor.
 */
typedef axl_bool (*TurbulenceLoopOnRead) (TurbulenceLoop * loop, 
					  TurbulenceCtx  * ctx,
					  int              descriptor, 
					  axlPointer       ptr, 
					  axlPointer       ptr2);

TurbulenceLoop * turbulence_loop_create (TurbulenceCtx * ctx);

void             turbulence_loop_set_read_handler (TurbulenceLoop        * loop,
						   TurbulenceLoopOnRead    on_read,
						   axlPointer              ptr,
						   axlPointer              ptr2);

void             turbulence_loop_watch_descriptor (TurbulenceLoop        * loop,
						   int                     descriptor,
						   TurbulenceLoopOnRead    on_read,
						   axlPointer              ptr,
						   axlPointer              ptr2);

void             turbulence_loop_close (TurbulenceLoop * loop, 
					 axl_bool        notify);

#endif

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
#include <turbulence.h>

/* local include */
#include <turbulence-ctx-private.h>

/* 
 * @internal Context variable used to hold current turbulence status.
 */
TurbulenceCtx * turbulence_ctx_global = NULL;

/** 
 * @brief Allows to create a new turbulence context (an object used by
 * the turbulence runtime to hold its current run time configuration).
 * 
 * @return A newly allocated reference to the Turbulence context
 * created. This function is already called by the turbulence engine.
 */
TurbulenceCtx * turbulence_ctx_new ()
{
	TurbulenceCtx * ctx;

	/* create the context */
	ctx      = axl_new (TurbulenceCtx, 1);
	ctx->pid = -1;
	vortex_mutex_create (&ctx->exit_mutex);

	/* db list */
	vortex_mutex_create (&ctx->db_list_mutex);
	ctx->db_list_opened = axl_list_new (turbulence_db_list_equal, (axlDestroyFunc) turbulence_db_list_close_internal);

	/* return context created */
	return ctx;
}

/** 
 * @brief Allows to configure the turbulence context to be used.
 * 
 * @param ctx The context reference to use.
 */
void            turbulence_ctx_set (TurbulenceCtx * ctx)
{
	/* do nothing if a null context is received */
	if (ctx == NULL)
		return;

	/* configure context */
	turbulence_ctx_global = ctx;

	return;
}

/** 
 * @brief Allows to get a reference to the current context configured.
 *
 * @return Reference to the current context configured.
 */
TurbulenceCtx * turbulence_ctx_get ()
{
	/* return current context configured */
	return turbulence_ctx_global;
}

/** 
 * @brief Deallocates the turbulence context provided.
 * 
 * @param ctx The context reference to terminate.
 */
void            turbulence_ctx_free (TurbulenceCtx * ctx)
{
	/* do not perform any operation */
	if (ctx == NULL)
		return;

	/* db list */
	vortex_mutex_destroy (&ctx->db_list_mutex);
	axl_list_free (ctx->db_list_opened);
	ctx->db_list_opened = NULL;

	axl_dtd_free (ctx->db_list_dtd);
	ctx->db_list_dtd = NULL;

	/* free mutex */
	vortex_mutex_destroy (&ctx->exit_mutex);

	/* release the node itself */
	axl_free (ctx);

	return;
}



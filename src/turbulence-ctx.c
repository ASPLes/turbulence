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
#include <turbulence.h>

/* local include */
#include <turbulence-ctx-private.h>

/** 
 * \defgroup turbulence_ctx Turbulence Context: API provided to handle Turbulence contexts
 */

/** 
 * \addtogroup turbulence_ctx
 * @{
 */

/** 
 * @brief Allows to create a new turbulence context (an object used by
 * the turbulence runtime to hold its current run time state). 
 *
 * The idea behind the turbulence initialization is to create a
 * context object and the call to \ref turbulence_init with that
 * context object to create a new run-time. This function also calls
 * to init the vortex context associated. You can get it with \ref
 * turbulence_ctx_get_vortex_ctx.
 *
 * Once required to finish the context created a call to \ref
 * turbulence_exit is required, followed by a call to \ref
 * turbulence_ctx_free.
 * 
 * @return A newly allocated reference to the Turbulence context
 * created. This function is already called by the turbulence engine.
 */
TurbulenceCtx * turbulence_ctx_new ()
{
	TurbulenceCtx * ctx;

	/* create the context */
	ctx        = axl_new (TurbulenceCtx, 1);

	/* create hash */
	ctx->data  = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	vortex_mutex_create (&ctx->data_mutex);

	/* set log descriptors to something not usable */
	ctx->general_log = -1;
	ctx->error_log   = -1;
	ctx->access_log  = -1;
	ctx->vortex_log  = -1;

	/* init ppath unique id assigment */
	ctx->ppath_next_id = 1;

	/* init wait queue */
	ctx->wait_queue    = vortex_async_queue_new ();

	/* return context created */
	return ctx;
}

/** 
 * @internal Allows to reinit internal state of the context process
 * after a child process creation. It also closed or removes internal
 * elements not required by child process.
 */
void           turbulence_ctx_reinit (TurbulenceCtx * ctx, TurbulenceChild * child, TurbulencePPathDef * def)
{
	/* init pid to the child */
	ctx->pid = getpid ();

	/* define context child and child context */
	ctx->child   = child;
	child->ctx   = ctx;

	/* record profile path selected */
	ctx->child->ppath = def;

	/* re-init mutex */
	vortex_mutex_create (&ctx->exit_mutex);
	vortex_mutex_create (&ctx->db_list_mutex);
	vortex_mutex_create (&ctx->data_mutex);
	vortex_mutex_create (&ctx->registered_modules_mutex);

	/* mutex on child object */
	vortex_mutex_create (&ctx->child->mutex);

	/* clean child process list: reinit = axl_true */
	turbulence_process_init (ctx, axl_true);

	return;
}

/** 
 * @brief Allows to configure the vortex context (VortexCtx)
 * associated to the provided \ref TurbulenceCtx.
 * 
 * @param ctx \ref TurbulenceCtx to be configured.
 * @param vortex_ctx Vortex context to be configured.
 */
void            turbulence_ctx_set_vortex_ctx (TurbulenceCtx * ctx, 
					       VortexCtx     * vortex_ctx)
{
	v_return_if_fail (ctx);

	/* acquire a reference to the context */
	vortex_ctx_ref (vortex_ctx);
	
	/* configure vortex ctx */
	ctx->vortex_ctx = vortex_ctx;

	/* configure reference on vortex ctx */
	vortex_ctx_set_data (vortex_ctx, "tbc:ctx", ctx);

	return;
}

/** 
 * @brief Allows to get the \ref TurbulenceCtx associated to the
 * vortex.
 * 
 * @param ctx The turbulence context where it is expected to find a
 * Vortex context (VortexCtx).
 * 
 * @return A reference to the VortexCtx.
 */
VortexCtx     * turbulence_ctx_get_vortex_ctx (TurbulenceCtx * ctx)
{
	v_return_val_if_fail (ctx, NULL);

	/* return the vortex context associated */
	return ctx->vortex_ctx;
}

/** 
 * @brief Allows to configure user defined data indexed by the
 * provided key, associated to the \ref TurbulenceCtx.
 * 
 * @param ctx The \ref TurbulenceCtx to configure with the provided data.
 *
 * @param key The index string key under which the data will be
 * retreived later using \ref turbulence_ctx_get_data. The function do
 * not support storing NULL keys.
 *
 * @param data The user defined pointer to data to be stored. If NULL
 * is provided the function will understand it as a removal request,
 * calling to delete previously stored data indexed by the same key.
 */
void            turbulence_ctx_set_data       (TurbulenceCtx * ctx,
					       const char    * key,
					       axlPointer      data)
{
	v_return_if_fail (ctx);
	v_return_if_fail (key);

	/* acquire the mutex */
	vortex_mutex_lock (&ctx->data_mutex);

	/* perform a simple insert */
	axl_hash_insert (ctx->data, (axlPointer) key, data);

	/* release the mutex */
	vortex_mutex_unlock (&ctx->data_mutex);

	return;
}


/** 
 * @brief Allows to configure user defined data indexed by the
 * provided key, associated to the \ref TurbulenceCtx, with optionals
 * destroy handlers.
 *
 * This function is quite similar to \ref turbulence_ctx_set_data but
 * it also provides support to configure a set of handlers to be
 * called to terminate data associated once finished \ref
 * TurbulenceCtx.
 * 
 * @param ctx The \ref TurbulenceCtx to configure with the provided data.
 *
 * @param key The index string key under which the data will be
 * retreived later using \ref turbulence_ctx_get_data. The function do
 * not support storing NULL keys.
 *
 * @param data The user defined pointer to data to be stored. If NULL
 * is provided the function will understand it as a removal request,
 * calling to delete previously stored data indexed by the same key.
 *
 * @param key_destroy Optional handler to destroy key stored.
 *
 * @param data_destroy Optional handler to destroy value stored.
 */
void            turbulence_ctx_set_data_full  (TurbulenceCtx * ctx,
					       const char    * key,
					       axlPointer      data,
					       axlDestroyFunc  key_destroy,
					       axlDestroyFunc  data_destroy)
{
	v_return_if_fail (ctx);
	v_return_if_fail (key);

	/* acquire the mutex */
	vortex_mutex_lock (&ctx->data_mutex);

	/* perform a simple insert */
	axl_hash_insert_full (ctx->data, (axlPointer) key, key_destroy, data, data_destroy);

	/* release the mutex */
	vortex_mutex_unlock (&ctx->data_mutex);

	return;
}

/** 
 * @brief Allows to retrieve data stored by \ref
 * turbulence_ctx_set_data and \ref turbulence_ctx_set_data_full.
 * 
 * @param ctx The \ref TurbulenceCtx where the retrieve operation will
 * be performed.
 *
 * @param key The key that index the data to be returned.
 * 
 * @return A reference to the data or NULL if nothing was found. The
 * function also returns NULL if a NULL ctx is received.
 */
axlPointer      turbulence_ctx_get_data       (TurbulenceCtx * ctx,
					       const char    * key)
{
	axlPointer data;
	v_return_val_if_fail (ctx, NULL);

	/* acquire the mutex */
	vortex_mutex_lock (&ctx->data_mutex);

	/* perform a simple insert */
	data = axl_hash_get (ctx->data, (axlPointer) key);

	/* release the mutex */
	vortex_mutex_unlock (&ctx->data_mutex);

	return data;
}

/** 
 * @brief Allows to implement a microseconds blocking wait.
 *
 * @param ctx The context where the wait will be implemented.
 *
 * @param microseconds Blocks the caller during the value
 * provided. 1.000.000 = 1 second.
 * 
 * If ctx is NULL or microseconds <= 0, the function returns
 * inmediately. 
 */
void            turbulence_ctx_wait           (TurbulenceCtx * ctx,
					       long microseconds)
{
	if (ctx == NULL || microseconds <= 0) 
		return;
	/* acquire a reference */
	msg2 ("Process waiting during %d microseconds..", (int) microseconds);
	vortex_async_queue_timedpop (ctx->wait_queue, microseconds);
	return;
}

/** 
 * @brief Allows to check if the provided turbulence ctx is associated
 * to a child process.
 *
 * This function can be used to check if the current execution context
 * is bound to a child process which means we are running in a child
 * process
 *
 * @return axl_false when contexts is representing master process
 * otherwise axl_true is returned (child process). Keep in mind the
 * function returns axl_false (master process) in the case or NULL
 * reference received.
 */
axl_bool        turbulence_ctx_is_child       (TurbulenceCtx * ctx)
{
	if (ctx == NULL)
		return axl_false;
	return ctx->child != NULL;
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

	/* terminate hash */
	axl_hash_free (ctx->data);
	ctx->data = NULL;
	vortex_mutex_destroy (&ctx->data_mutex);

	/* release wait queue */
	vortex_async_queue_unref (ctx->wait_queue);

	/* now modules and vortex library is stopped, terminate
	 * modules unloading them. This will allow having usable code
	 * mapped into modules address which is usable until the last
	 * time.  */
	turbulence_module_cleanup (ctx); 

	/* include a error warning */
	if (ctx->vortex_ctx) {
		if (vortex_ctx_ref_count (ctx->vortex_ctx) <= 0) 
			error ("ERROR: current process is attempting to release vortex context more times than references supported");
		else {
			/* release vortex reference acquired */
			msg ("Finishing VortexCtx (%p, ref count: %d)", ctx->vortex_ctx, vortex_ctx_ref_count (ctx->vortex_ctx));
			vortex_ctx_unref (&(ctx->vortex_ctx));
		}
	}

	/* release the node itself */
	msg ("Finishing TurbulenceCtx (%p)", ctx);
	axl_free (ctx);

	return;
}

/* @} */

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
#include <turbulence-child.h>

/* local private include */
#include <turbulence-ctx-private.h>

/** 
 * @internal Creates a object that will represents a child object and
 * initialize internal data structures for its function.
 */
TurbulenceChild * turbulence_child_new (TurbulenceCtx * ctx, TurbulencePPathDef * def)
{
	TurbulenceChild * result;
	char            * temp_dir;

	result        = axl_new (TurbulenceChild, 1);
	/* check allocation */
	if (result == NULL)
		return NULL;

	/* flag child context as not started (until turbulence-process
	 * flag it as started): this is used to control clean start */
	ctx->started = axl_false;

	/* create socket path: socket used to transfer file descriptors from parent to child */
	result->socket_control_path = axl_strdup_printf ("%s%s%s%s%p.tbc",
							 turbulence_runtime_datadir (ctx),
							 VORTEX_FILE_SEPARATOR,
							 "turbulence",
							 VORTEX_FILE_SEPARATOR,
							 result);
	/* check result */
	if (result->socket_control_path == NULL) {
		axl_free (result);
		return NULL;
	} /* end if */

	/* now check check base dir for socket control path exists */
	temp_dir = turbulence_base_dir (result->socket_control_path);
	if (temp_dir && ! vortex_support_file_test (temp_dir, FILE_EXISTS)) {
		/* base directory having child socket control do not exists */
		wrn ("run time directory %s do not exists, creating..", temp_dir);
		if (! turbulence_create_dir (temp_dir)) {
			/* call to finish references */
			turbulence_child_unref (result);

			error ("Unable to create directory to hold child socket connection, unable to create child process..");
			axl_free (temp_dir);
			return NULL;
		} /* end if */
	} /* end if */
	/* free temporal directory */
	axl_free (temp_dir);
	
	/* set profile path and context */
	result->ppath = def;
	result->ctx   = ctx;

	/* create listener connection used for child management */
	result->conn_mgr = vortex_listener_new_full2 (ctx->vortex_ctx, "0.0.0.0", "0", axl_false, NULL, NULL);
	if (! vortex_connection_is_ok (result->conn_mgr, axl_false)) {
		error ("Failed to connection child connection management, unable to create child process");

		/* shutdown connection to be child by the child */
		vortex_connection_close (result->conn_mgr);
		axl_free (result);

		return NULL;
	}

	/* unregister conn mgr */
	turbulence_conn_mgr_unregister (ctx, result->conn_mgr);
	
	/* set default reference counting */
	result->ref_count = 1;
	vortex_mutex_create (&result->mutex);

	return result;
}

/** 
 * @brief Allows to a acquire a reference to the provided object.
 *
 * @param child The child object to get a reference.
 *
 * @return axl_true if the reference was acquired otherwise, axl_false
 * is returned.
 */
axl_bool              turbulence_child_ref (TurbulenceChild * child)
{
	if (child == NULL || child->ref_count == 0)
		return axl_false;

	/* get mutex */
	vortex_mutex_lock (&child->mutex);

	child->ref_count++;

	/* release */
	vortex_mutex_unlock (&child->mutex);

	return axl_true;
}


/** 
 * @brief Allows to decrease reference counting and deallocating
 * resources when reference counting reaches 0.
 *
 * @param child A reference to the child to decrease reference counting.
 */
void              turbulence_child_unref (TurbulenceChild * child)
{
	TurbulenceCtx * ctx;

	if (child == NULL || child->ref_count == 0)
		return;

	/* get mutex */
	vortex_mutex_lock (&child->mutex);

	child->ref_count--;

	if (child->ref_count != 0) {
		/* release */
		vortex_mutex_unlock (&child->mutex);
		return;
	}

	/* get reference to the context */
	ctx = child->ctx;

#if defined(AXL_OS_UNIX)
	/* unlink (child->socket_control_path);*/
	axl_free (child->socket_control_path);
	child->socket_control_path = NULL;
	vortex_close_socket (child->child_connection);
#endif

	/* finish child connection */
	msg ("CHILD: Finishing child connection manager id=%d (refs: %d, role %d)", vortex_connection_get_id (child->conn_mgr), 
	     vortex_connection_ref_count (child->conn_mgr), vortex_connection_get_role (child->conn_mgr));
	
	/* release reference if it is either initiator or listener */
	vortex_connection_shutdown (child->conn_mgr);
	vortex_connection_unref (child->conn_mgr, "free data"); 

	/* nullify */
	child->conn_mgr = NULL;

	/* destroy mutex */
	vortex_mutex_destroy (&child->mutex);

	axl_free (child);

	return;
}


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
#include <turbulence.h>

/* local include */
#include <turbulence-ctx-private.h>

/**
 * \defgroup turbulence_db_list Turbulence Db List: common abstract interface to store list of items (flushed to the storage device).
 */

/**
 * \addtogroup turbulence_db_list
 * @{
 */

struct _TurbulenceDbList {
	axlDoc        * doc;
	axlNode       * first;
	char          * full_path;
	long int        last_modification;
	VortexMutex     mutex;

	/* context that loaded the list */
	TurbulenceCtx * ctx;
};

/** 
 * @internal Function that allows to compare two TurbulenceDbList
 * pointers.
 */
int turbulence_db_list_equal (axlPointer a, axlPointer b)
{
	TurbulenceDbList * itemA = a;
	TurbulenceDbList * itemB = b;

	/* check null references */
	if (itemA == NULL)
		return 1;

	if (itemB == NULL)
		return 1;
	
	/* return 0 if they are equal, 1 if not */
	return axl_cmp (itemA->full_path, itemB->full_path) ? 0 : 1;
}

/** 
 * @internal Function that allows to validate db-list xml files
 * loaded.  
 */
bool __turbulence_db_list_validate (TurbulenceCtx * ctx, axlDoc * doc, axlError **error)
{

	v_return_val_if_fail (ctx, false);

	/* check dtd status */
	if (ctx->db_list_dtd == NULL) {
		axl_error_new (-1, "tbc-dblist-mgr installation error, unable to find DTD to validate output", NULL, error);
		return false;
	}

	/* return dtd validation result */
	return axl_dtd_validate (doc, ctx->db_list_dtd, error);
}

/** 
 * @brief Allows to open the provide db list, containing a list of
 * tokens that follows the format provided by the module.
 *
 * Once the db list is opened, a reference to its handler is
 * provided. This handler is used to access and modify the content
 * with functions like:
 * 
 *  - \ref turbulence_db_list_exists
 *  - \ref turbulence_db_list_add
 *  - \ref turbulence_db_list_remove
 *
 * The reference returned will be automatically managed by
 * Turbulence. This means:
 * 
 * - It is not required to close the handler at the module close. This
 * is done by turbulence at the termination process, closing and
 * flushing all data to its proper destination. All resources
 * allocated are also freed.
 *
 * - It is not required to perform a reloading operation to ensure the
 * data loaded in memory is in sync with the storage
 * device. Turbulence will take care about file changes, reloading
 * them if necessary.
 *
 *
 * It is possible to force a reload operation by calling to: \ref
 * turbulence_db_list_reload. It is also possible to explicitly close
 * the handler opened by calling: \ref turbulence_db_list_close.
 *
 * The common case while using \ref TurbulenceDbList is to open the db
 * file at the module init function as:
 * \code
 * TurbulenceDbList * list;
 * axlError         * err;
 *
 * // open the db list, providing a reference to an axlError to get
 * // textual dianogtic error reporting and a list of tokens configuring
 * // the full path to the file to be opened, ended by a NULL decl.
 * list = turbulence_db_list_open (ctx, &err, SYSCONFDIR, 
 *                                       "turbulence", 
 *                                       "your-module", 
 *                                       "db-file.xml", NULL);
 * if (list == NULL) {
 *      error ("Something have failed: %s", axl_error_get (err));
 *      return false;
 * }
 * \endcode
 *
 * If the file provided to be opened doesn't exists, the function will
 * load an empty db list into memory that will be dumped to the
 * storage device once the db list is closed.
 *
 * The path to the db list must exist. The function will create all
 * directories in the path that doesn't exists.
 *
 * @param ctx Turbulence context where the operation will take place.
 *
 * @param error An optional user pointer to an axlError reference.
 *
 * @param token First token of a list of tokens configuring the full
 * path to the db list. This list must be terminated with a NULL decl.
 * 
 * @return A referece to the \ref TurbulenceDbList handler. It is not
 * required to perform a close operation when no longer needed. This
 * is already done by Turbulence.
 */
TurbulenceDbList * turbulence_db_list_open   (TurbulenceCtx   * ctx,
					      axlError       ** error, 
					      const char      * token, 
					      ...)
{

	/* get turbulence context */
	va_list            args;
	char             * full_path;
	char             * aux;
	char             * aux2;
	TurbulenceDbList * list;

	/* check context */
	v_return_val_if_fail (ctx, NULL);

	/* check reference */
	if (token == NULL) {
		axl_error_new (-1, "Provided a null reference for the first token configuring the path to open", NULL, error);
		return NULL;
	}
	
	/* build reference */
	list      = axl_new (TurbulenceDbList, 1);

	/* build full path to the file */
	va_start (args, token);
	aux       = va_arg (args, char *);
	full_path = axl_strdup (token);
	list->ctx = ctx;

	while (aux != NULL) {

		/* check if the current path exists */
		if (! vortex_support_file_test (full_path, FILE_EXISTS)) {
			if (! turbulence_create_dir (full_path)) {
				error ("failed to create directory: %s", full_path);
			} else {
				msg ("directory created: %s", full_path);
			} /* end if */
		} /* end if */

		/* first path */
		aux2      = full_path;
		full_path = axl_strdup_printf ("%s%s%s", full_path, VORTEX_FILE_SEPARATOR, aux);
		axl_free (aux2);

		/* next token */
		aux = va_arg (args, char *);
	} /* end while */
	
	va_end (args);

	msg2 ("opening db-list [xml backend]: %s", full_path);

	/* configure the path */
	list->full_path = full_path;
	
	/* check if the file exists */
	if (vortex_support_file_test (list->full_path, FILE_EXISTS)) {
		/* open the file  */
		list->doc = axl_doc_parse_from_file (list->full_path, error);
		if (list->doc == NULL) {
			/* free handler */
			turbulence_db_list_close (list);
			return NULL;
		} /* end if */

		/* validate the list */
		if (! __turbulence_db_list_validate (ctx, list->doc, error)) {
			/* free handler */
			turbulence_db_list_close (list);
			return NULL;
		} 

		/* set the first node in the document */
		list->first = axl_doc_get_root (list->doc);
		if (list->first != NULL)
			list->first = axl_node_get_first_child (list->first);

		/* get last modification value */
		list->last_modification = turbulence_last_modification (list->full_path);

	} else {
		msg ("db list file not found, creating one: %s", full_path);

		/* file not found, open a new one */
		list->doc = axl_doc_create ("1.0", NULL, true);
		axl_doc_set_root (list->doc, axl_node_create ("turbulence-db-list"));

	} /* end if */
	
	/* init its mutex */
	vortex_mutex_create (&(list->mutex));

	/* add the db list to the list of files opened */
	vortex_mutex_lock (&ctx->db_list_mutex);

	/* add the db list opened */
	axl_list_append (ctx->db_list_opened, list);
	msg2 ("added list %p to axlList %p, current count: %d", 
	      list, ctx->db_list_opened, axl_list_length (ctx->db_list_opened));

	/* unlock and return */
	vortex_mutex_unlock (&ctx->db_list_mutex);

	/* return reference */
	return list;
}

/** 
 * @brief Allows to check if the provided value is already added in
 * the provided db list.
 * 
 * @param list The list where the value will be checked.
 *
 * @param value The value to be checked. The list can't contain null
 * values, so requesting with NULL will always return false. However
 * empty values are supported ("").
 * 
 * @return true if the list contains the value provided (first
 * reference), otherwise false is returned.
 */
bool               turbulence_db_list_exists (TurbulenceDbList * list,
					      const char       * value)
{
	axlNode * node;

	/* check values received */
	if (list == NULL)
		return false;
	if (value == NULL)
		return false;

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	node = list->first;
	while (node != NULL) {
		
		/* check the value */
		if (axl_cmp (value, ATTR_VALUE (node, "value"))) {
			/* value found, unlock and return */
			vortex_mutex_unlock (&(list->mutex));

			return true;
		} /* end if */

		/* get next node */
		node = axl_node_get_next_called (node, "item");
		
	} /* end while */

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));
	
	return false;
}

/** 
 * @brief Allows to add the content provided to the db list. The
 * function doesn't check if the value to be added already exists.
 * After the function finish, the content provided will be added to
 * the in memory representation and the storage device.
 * 
 * @param list The db list where the content will be added.
 *
 * @param value The value to add to the list.
 * 
 * @return true if the item was properly added, false if an error was
 * found.
 */
bool               turbulence_db_list_add    (TurbulenceDbList * list,
					      const char       * value)
{
	axlNode * node;

	/* check values received */
	v_return_val_if_fail (list && value, false);

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	node = list->first;

	if (node == NULL) {
		/* basic case, first item added */
		list->first = axl_node_create ("item");
		axl_node_set_attribute (list->first, "value", value);
		
		/* check the root node */
		node = axl_doc_get_root (list->doc);
		axl_node_set_child (node, list->first);

	} else {
		/* usual case, add it at the end */
		node = axl_node_create ("item");
		axl_node_set_attribute (node, "value", value);

		/* add the content */
		axl_node_set_child (axl_node_get_parent (list->first), node);
	} /* end if */

	/* update first node */
	list->first = axl_doc_get_root (list->doc);
	if (list->first != NULL)
		list->first = axl_node_get_first_child (list->first);

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));

	/* flush the content */
	return turbulence_db_list_flush (list);
}

/** 
 * @brief Allows to remove the provided content (first reference) on
 * the provided db list.
 * 
 * @param list The reference of the db list where the item will be
 * deleted.
 *
 * @param value The string value to be removed from the db list.
 * 
 * @return true if the item was removed, otherwise false is returned.
 */
bool               turbulence_db_list_remove (TurbulenceDbList * list,
					      const char       * value)
{

	axlNode * node;

	/* check values received */
	if (list == NULL)
		return false;
	if (value == NULL)
		return false;

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	node = list->first;
	while (node != NULL) {
		
		/* check the item */
		if (axl_cmp (value, ATTR_VALUE (node, "value"))) {
			/* found the node holding the value */
			axl_node_remove (node, true);

			/* update first node */
			list->first = axl_doc_get_root (list->doc);
			if (list->first != NULL)
				list->first = axl_node_get_first_child (list->first);

			/* unlock and flush */
			vortex_mutex_unlock (&(list->mutex));

			/* flush */
			turbulence_db_list_flush (list);

			return true;
		} /* end if */

		/* get next node */
		node = axl_node_get_next_called (node, "item");
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));

	/* item removed either because it wasn't found or because it
	 * was really removed */
	return true;
}

/** 
 * @brief Allows to produce a remove operation (or several remove
 * operation) using an external function which is called to check if
 * the item should be removed or not.
 * 
 * @param list The list to be filtered by removing all items selected by the provided function.
 *
 * @param func A handler configure to check if an item must be removed
 * or not. If the function returns true the item will be
 * removed. Otherwise it will remain.
 *
 * @param user_data User defined pointer which is passed to the filter
 * (remove) function.
 * 
 * @return true if the operation was fully completed, otherwise false
 * is returned.
 */
bool               turbulence_db_list_remove_by_func (TurbulenceDbList           * list,
						      TurbulenceDbListRemoveFunc   func,
						      axlPointer                   user_data)
{
	axlNode * node;
	axlNode * nodeAux;

	/* check values received */
	if (list == NULL)
		return false;
	if (func == NULL)
		return false;

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	node = list->first;
	while (node != NULL) {
		
		/* check the item */
		if (func (ATTR_VALUE (node, "value"), user_data)) {

			/* get next node */
			nodeAux = axl_node_get_next_called (node, "item");
			
			/* found the node holding the value */
			axl_node_remove (node, true);

			/* update first node */
			list->first = axl_doc_get_root (list->doc);
			if (list->first != NULL)
				list->first = axl_node_get_first_child (list->first);

			/* update node */
			node = nodeAux;
			continue;

		} /* end if */

		/* get next node */
		node = axl_node_get_next_called (node, "item");
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));

	/* item removed either because it wasn't found or because it
	 * was really removed */
	return true;
}

/** 
 * @brief Allows to implement an edit operation on the provided dblist
 * in an atomic way.
 *
 * This function could be replaced by a call to \ref
 * turbulence_db_list_remove followed by a call to \ref
 * turbulence_db_list_add. However, there is a race condition between
 * those two calls, which is avoided by this function. This function
 * also allows to conserve position of the previous item edited, which
 * is likely to be lost by an remove/add pattern is used.
 *
 * @param list The list to be edited.
 *
 * @param oldValue The element to be edited. This element will be
 * searched and edited/replaced with the value provided as newValue.
 *
 * @param newValue The new value to place in exchange for oldValue.
 * 
 * @return true if the operation was properly completed, otherwise
 * false is returned.
 */
bool               turbulence_db_list_edit   (TurbulenceDbList * list,
					      const char       * oldValue,
					      const char       * newValue)
{
	axlNode * node;

	/* check values received */
	if (list == NULL)
		return false;
	if (oldValue == NULL)
		return false;
	if (newValue == NULL)
		return false;

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	node = list->first;
	while (node != NULL) {
		
		/* check the item */
		if (axl_cmp (oldValue, ATTR_VALUE (node, "value"))) {
			/* found the node holding the value, replace
			 * the attribute with the new value */
			axl_node_remove_attribute (node, "value");
			axl_node_set_attribute (node, "value", newValue);

			/* unlock and flush */
			vortex_mutex_unlock (&(list->mutex));

			/* flush */
			turbulence_db_list_flush (list);

			return true;
		} /* end if */

		/* get next node */
		node = axl_node_get_next_called (node, "item");
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));

	/* item removed either because it wasn't found or because it
	 * was really removed */
	return true;
}

/** 
 * @brief Allows to get a copy translated into a axlList from the
 * current content held by the \ref TurbulenceDbList.
 *
 * The function is useful to get a list with the content inside the db
 * list, especially for view operations. The list returned have the
 * content at the particular time the call was done, and any
 * modification to the list have no effect on the stored data.
 * 
 * @param list The db list requested to return its content.
 * 
 * @return A reference to a newly allocated axlList, containing
 * strings representing the content hold by the provided db list. The
 * reference returned is owned by the caller and a call to
 * axl_list_free is required when no longer needed the data returned.
 */
axlList          * turbulence_db_list_get    (TurbulenceDbList * list)
{
	axlNode * node;
	axlList * result;

	/* check values received */
	if (list == NULL)
		return false;

	/* reload the document */
	turbulence_db_list_reload (list);
	
	/* lock */
	vortex_mutex_lock (&(list->mutex));

	/* get the first node */
	result = axl_list_new (axl_list_always_return_1, axl_free);
	node   = list->first;
	while (node != NULL) {
		
		/* store a copy of the item */
		axl_list_add (result, axl_strdup (ATTR_VALUE (node, "value")));
		
		/* get next node */
		node = axl_node_get_next_called (node, "item");
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&(list->mutex));

	/* return the list created */
	return result;
}

/** 
 * @brief Allows to close the db list opened.
 * 
 * @param list The list to close.
 * 
 * @return true if the list was properly close, otherwise false is
 * returned. 
 */
bool               turbulence_db_list_close  (TurbulenceDbList * list)
{
	TurbulenceCtx * ctx;

	/* if a null reference is received do not perform any
	 * operation, and return ok status. */
	if (list == NULL)
		return true;

	/* get a reference to the context */
	ctx = list->ctx;

	/* remove the list from the opened db list */
	vortex_mutex_lock (&ctx->db_list_mutex);

	/* remove the item from the list without deallocating */
	axl_list_unlink_ptr (ctx->db_list_opened, list);

	/* clear memmory associated */
	turbulence_db_list_close_internal (list);

	/* unlock and return */
	vortex_mutex_unlock (&ctx->db_list_mutex);
	
	return true;
}

/** 
 * @brief Allows to close the db list opened.
 * 
 * @param list The list to close.
 * 
 * @return true if the list was properly close, otherwise false is
 * returned. 
 */
bool               turbulence_db_list_close_internal  (TurbulenceDbList * list)
{
	TurbulenceCtx * ctx;

	/* if a null reference is received do not perform any
	 * operation, and return ok status. */
	v_return_val_if_fail (list, true);

	/* get turbulence context */
	ctx = list->ctx;

	msg2 ("closing list %p", list);

	/* dump the document content */
	if (list->doc != NULL) {
		if (! axl_doc_dump_pretty_to_file (list->doc, list->full_path, 4))
			error ("failed to dump: %s", list->full_path);
	} /* end if */

	/* dealloc */
	axl_doc_free (list->doc);
	axl_free (list->full_path);
	vortex_mutex_destroy (&(list->mutex));
	axl_free (list);
	

	return true;
}

/** 
 * @brief Performs a reload operation, checking if the data in the
 * storage device have differences with actual memory loaded data.
 * 
 * @param list The reference to reload.
 * 
 * @return true if the reload operation was done, otherwise false is
 * returned.
 */
bool               turbulence_db_list_reload (TurbulenceDbList * list)
{
	TurbulenceCtx  * ctx;
	axlDoc         * newContent;
	axlDoc         * temp;
	axlError       * err;

	/* do nothing if null reference is received. */
	if (list == NULL)
		return true;
	
	/* get a reference */
	ctx = list->ctx;

	/* check last modification value and do nothing if nothing
	 * have changed  */
	if (turbulence_last_modification (list->full_path) == list->last_modification) {
		return true;
	}

	/* check if the document exists, and do no try to reload
	 * something is missing .. */
	if (! turbulence_file_test_v (list->full_path, FILE_EXISTS)) {
		return true;
	}

	/* open the document */
	newContent = axl_doc_parse_from_file (list->full_path, &err);
	if (newContent == NULL) {
		error ("failed to open for reload: %s, error was: %s", 
		       list->full_path, axl_error_get (err));
		axl_error_free (err);
		return false;
	}

	/* check if we have diferences */
	if (axl_doc_are_equal (list->doc, newContent)) {
		/* both documents are equal, doing nothing */
		axl_doc_free (newContent);
		return true;
	}

	/* documents differs, lock the mutex associated to the list */
	vortex_mutex_lock (&(list->mutex));
	/* get a reference to the old document */
	temp      = list->doc;

	/* install the new reference */
	list->doc   = newContent;

	/* update first references */
	list->first = axl_doc_get_root (list->doc);
	if (list->first)
		list->first = axl_node_get_first_child (list->first);

	vortex_mutex_unlock (&(list->mutex));

	/* now free previous content */
	axl_doc_free (temp);
	
	return true;
} 

/** 
 * @brief Allows to force a "flush" operation for the provided db list
 * reference.
 *
 * The function will take the content of the in memory representation
 * saving it into the storage device. This operation is done
 * automatically by the \ref TurbulenceDbList API, however it can be
 * forced by calling to this function.
 * 
 * @param list The db list to be flush.
 * 
 * @return true if the flush operation was properly done, otherwise
 * false is returned.
 */
bool               turbulence_db_list_flush  (TurbulenceDbList * list)
{
	TurbulenceCtx * ctx;
	
	/* if a null reference is received do not perform any
	 * operation, and return ok status. */
	if (list == NULL)
		return true;

	/* get context */
	ctx = list->ctx;

	/* lock the mutex */
	vortex_mutex_lock (&(list->mutex));
	
	/* dump the document content */
	if (! axl_doc_dump_pretty_to_file (list->doc, list->full_path, 4)) {
		error ("failed to dump: %s", list->full_path);
		return false;
	}

	/* update last modification file */
	list->last_modification = turbulence_last_modification (list->full_path);

	/* unlock the mutex */
	vortex_mutex_unlock (&(list->mutex));

	return true;
}

/** 
 * @brief Allows to get the number of items stored on the provided
 * turbulence db list.
 * 
 * @param list Turbulence db-list that is being requested to return
 * the number of items stored.
 * 
 * @return The number or items stored or -1 it if fails.
 */
int               turbulence_db_list_count          (TurbulenceDbList * list)
{
	int       count = 0;
	axlNode * node;

	if (list == NULL)
		return -1;

	/* lock the mutex */
	vortex_mutex_lock (&(list->mutex));

	node = list->first;
	while (node != NULL) {
		
		/* update count */
		count++;
		
		/* get next node */
		node = axl_node_get_next_called (node, "item");
		
	} /* end while */

	/* unlock the mutex */
	vortex_mutex_unlock (&(list->mutex));

	return count;
	
}

/** 
 * @brief Service used to start the turbulence db list module.
 * @return True if the module was properly started, otherwise false is
 * returned.
 */
bool               turbulence_db_list_init (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	char             * file;
	axlError         * err;
	VortexCtx        * vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	/* check reference */
	v_return_val_if_fail (ctx, false);

	/* init global variables */
	vortex_mutex_create (&ctx->db_list_mutex);
	ctx->db_list_opened = axl_list_new (turbulence_db_list_equal, (axlDestroyFunc) turbulence_db_list_close_internal);
	msg2 ("Init context list: %p on context: %p..", ctx->db_list_opened, ctx);

	/* init dtd to validate data */
	if (ctx->db_list_dtd == NULL) {
		file             = vortex_support_domain_find_data_file (vortex_ctx, "turbulence-data", "db-list.dtd");
		ctx->db_list_dtd = axl_dtd_parse_from_file (file, &err);

		/* db list */
		if (ctx->db_list_dtd == NULL) {
			msg2 ("failed to open DTD: %s, error was: %s", file, axl_error_get (err));
			axl_error_free (err);
		} /* end if */
		axl_free (file);

		/* if not found, try to open it directly */
		if (ctx->db_list_dtd == NULL) {
			file             = vortex_support_build_filename (TBC_DATADIR, "turbulence", "db-list.dtd", NULL);
			ctx->db_list_dtd = axl_dtd_parse_from_file (file, &err);

			/* check the file */
			if (ctx->db_list_dtd == NULL) {
				msg2 ("failed to open DTD: %s, error was: %s", file, axl_error_get (err));
				axl_error_free (err);
			} /* end if */
			axl_free (file);
		} /* end if */

		if (ctx->db_list_dtd == NULL) {
			msg ("failed to open DTD file to validate db-list content");
			return false;
		}

	} /* end if */

	return true;
}

/** 
 * @brief Allows to cleanup the turbulence db-list module. This call
 * is usually done from the \ref turbulence_init function, but it may
 * be used directly from those modules that only makes use from the
 * db-list by only calling to \ref turbulence_db_list_init.
 * 
 * @param ctx The context to cleanup.
 */
void               turbulence_db_list_cleanup (TurbulenceCtx * ctx)
{
	/* do not perform any operation if null is received */
	if (ctx == NULL)
		return;

	/* clean mutex */
	vortex_mutex_destroy (&ctx->db_list_mutex);

	/* clean list */
	msg ("cleaning up turbulence db list..");
	axl_list_free (ctx->db_list_opened);
	ctx->db_list_opened = NULL;
	
	/* clean dtd */
	axl_dtd_free (ctx->db_list_dtd);
	ctx->db_list_dtd = NULL;

	return;
}

/** 
 * @internal Service used to reload the module (reloading all db list
 * opened).
 * @return true if the operation was properly done, otherwise false is
 * returned.
 */
bool               turbulence_db_list_reload_module  ()
{
	return true;
}



/* @} */

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
 *         info@aspl.es - http://fact.aspl.es
 */
#include <turbulence-ppath.h>
#include <pcre.h>

typedef enum {
	PROFILE_ALLOW, 
	PROFILE_IF
} TurbulencePPathItemType;

/** 
 * @internal The following is a definition that allows representing an
 * expression either as an string (char) or a perl regular expression
 * (pcre), according to the compilation status.
 */
typedef 
#if defined(ENABLE_PCRE_SUPPORT)
pcre
#else
char
#endif
expr;

typedef struct _TurbulencePPathItem TurbulencePPathItem;

struct _TurbulencePPathItem {
	/* The type of the profile item path  */
	TurbulencePPathItemType type;
	
	/* support for the profile to be matched by this profile item
	 * path */
	expr * profile;

	/* Another list for all profile path item found inside this
	 * profile path item. This is only used by PROFILE_IF items */
	TurbulencePPathItem ** ppath_item;
	
};

typedef struct _TurbulencePPathDef {
	/* the name of the profile path group (optional value) */
	char * path_name;

	/* the server name pattern to be used to match the profile
	 * path. If turbulence wasn't built with pcre support, it will
	 * compiled as an string. */
	expr * serverName;

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	expr * src;

	/* a reference to the list of profile path supported */
	TurbulencePPathItem ** ppath_item;
	
} TurbulencePPathDef;

typedef struct _TurbulencePPath {
	/* list of profile paths found */
	TurbulencePPathDef ** items;
	
} TurbulencePPath;

TurbulencePPath * turbulence_paths = NULL;

/**
 * @internal Support function to parse the expression provided or to
 * just copy the string if turbulence wasn't built without pcre
 * support.
 */
axlPointer __turbulence_ppath_compile_expr (const char * expression, const char * error_msg)
{
#if defined(ENABLE_PCRE_SUPPORT)
	pcre       * result;
	const char * error;
	int          erroroffset;
	
	/* compile expression */
	result = pcre_compile (
		/* the pattern to compile */
		expression, 
		/* no flags */
		0,
		/* provide a reference to the error reporting */
		&error, &erroroffset, NULL);
	if (result == NULL) {
		/* failed to parse server name matching */
		error ("%s: %s, error: %s, at: %d",
		       error_msg, expression, error, erroroffset);
		return NULL;
	} /* end if */

	/* return expression */
	return result;
#else
	/* just copy the expression */
	return axl_strdup (expression);
#endif

} /* end __turbulence_ppath_compile_expr */

/** 
 * @internal Implementation that free the received expression
 * according to the compilation process.
 */
void __turbulence_ppath_free_expr (axlPointer expr)
{
	if (expr == NULL)
		return;
#if defined(ENABLE_PCRE_SUPPORT)
	pcre_free (expr);
#else
	axl_free (expr);
#endif
	return;
}

TurbulencePPathItem * __turbulence_ppath_get_item (axlNode * node)
{
	axlNode             * child;
	int                   iterator;
	TurbulencePPathItem * result = axl_new (TurbulencePPathItem, 1);

	/* get the profile expression */
	result->profile = __turbulence_ppath_compile_expr (
		/* the expression */
		ATTR_VALUE (node, "profile"), 
		/* error message */
		"Failed to get profile expression");
	if (result->profile == NULL) {
		axl_free (result);
		return NULL;
	} /* end if */

	/* configure the profile path item type */
	if (NODE_CMP_NAME (node, "allow")) {
		result->type = PROFILE_ALLOW;
	}else if (NODE_CMP_NAME (node, "if-success")) {
		result->type = PROFILE_IF;
	} /* end if */

	/* parse here childs inside <if-success> node */
	if (result->type == PROFILE_IF) {
		/* parse child nodes */
		result->ppath_item = axl_new (TurbulencePPathItem *, axl_node_get_child_num (node) + 1);
		child              = axl_node_get_first_child (node);
		iterator           = 0;
		while (child != NULL) {
			/* get the first definition */
			result->ppath_item[iterator] = __turbulence_ppath_get_item (child);
			
			/* next profile path item */
			child = axl_node_get_next (node);
			iterator++;
			
		} /* end if */
	} /* end if */
	
	/* return result parsed */
	return result;
}

/** 
 * @internal Prepares the runtime execution to provide profile path
 * support according to the current configuration.
 * 
 */
bool turbulence_ppath_init ()
{
	axlNode            * node; 
	axlNode            * pdef;
	TurbulencePPathDef * definition;
	int                  iterator;
	int                  iterator2;
	
	/* parse all profile path configurations */
	pdef = axl_doc_get (turbulence_config_get (), "/turbulence/profile-path-configuration/path-def");
	if (pdef == NULL) {
		error ("No profile path configuration was found, you must set at least one profile path.");
		return false;
	} /* end if */
	
	/* get the parent node */
	node = axl_node_get_parent (pdef);
	
	/* create the turbulence ppath */
	turbulence_paths        = axl_new (TurbulencePPath, 1);
	turbulence_paths->items = axl_new (TurbulencePPathDef *, axl_node_get_child_num (node) + 1);

	/* now parse each profile path def found */
	iterator = 0;
	while (pdef != NULL) {

		/* get the reference to the profile path */
		turbulence_paths->items[iterator] = axl_new (TurbulencePPathDef, 1);
		definition                        = turbulence_paths->items[iterator];

		/* catch all data from the profile path def header */
		if (HAS_ATTR (pdef, "path-name")) {
			/* catch the ppath name */
			definition->path_name = axl_strdup (ATTR_VALUE (pdef, "path-name"));
		} /* end if */

		/* catch server name match */
		if (HAS_ATTR (pdef, "server-name")) {
			definition->serverName = __turbulence_ppath_compile_expr (ATTR_VALUE (pdef, "server-name"),
										  "Failed to parse \"server-name\" expression at profile def");
		} /* end if HAS_ATTR (pdef, "server-name")) */

		if (HAS_ATTR (pdef, "src")) {
			definition->src = __turbulence_ppath_compile_expr (ATTR_VALUE (pdef, "src"),
									   "Failed to parse \"src\" expression at profile def");
		} /* end if (HAS_ATTR (pdef, "src")) */

		/* now, we have to parse all childs. Rules from the
		 * same level are chosable at the same time. If the
		 * expression found is an <if-success>, it is managed
		 * as an <allow> node, but providing more content if
		 * the profile negotiation success.  */
		node                   = axl_node_get_first_child (pdef);
		iterator2              = 0;
		definition->ppath_item = axl_new (TurbulencePPathItem *, axl_node_get_child_num (pdef) + 1);
		while (node != NULL) {
			/* get the first definition */
			definition->ppath_item[iterator2] = __turbulence_ppath_get_item (node);
			
			/* next profile path item */
			node = axl_node_get_next (node);
			iterator2++;
			
		} /* end if */
		
		/* get next profile path def */
		iterator++;
		pdef = axl_node_get_next (pdef);
	} /* end while */
	
	msg ("profile path definition ok..");

	/* return ok code */
	return true;
}

void __turbulence_ppath_free_item (TurbulencePPathItem * item)
{
	int iterator;

	/* free profile expression */
	__turbulence_ppath_free_expr (item->profile);

	iterator = 0;
	while (item->ppath_item != NULL && item->ppath_item[iterator]) {
		/* free the profile path */
		__turbulence_ppath_free_item (item->ppath_item[iterator]);

		/* next profile path item */
		iterator++;
	} /* end if */

	/* free the array */
	axl_free (item->ppath_item);

	/* free the item */
	axl_free (item);

	return;
}

/** 
 * @internal Terminates the profile path module, cleanup all memory
 * used.
 */
void turbulence_ppath_cleanup ()
{
	int iterator;
	int iterator2;
	TurbulencePPathDef * def;

	/* terminate profile paths */
	if (turbulence_paths != NULL) {

		/* for each profile path item iterator */
		iterator = 0;
		while (turbulence_paths->items[iterator] != NULL) {
			/* get the definition */
			def       = turbulence_paths->items[iterator];

			/* free profile path name definition */
			axl_free (def->path_name);
			__turbulence_ppath_free_expr (def->serverName);
			__turbulence_ppath_free_expr (def->src);

			iterator2 = 0;
			while (def->ppath_item[iterator2] != NULL) {
				
				/* free the item */
				__turbulence_ppath_free_item (def->ppath_item[iterator2]);

				/* next iterator */
				iterator2++;
			} /* end while */

			/* free the definition itself and its items */
			axl_free (def->ppath_item);
			axl_free (def);
			
			/* next profile path def */
			iterator++;
		} /* end if */

		/* free profile path array */
		axl_free (turbulence_paths->items);
		axl_free (turbulence_paths);
		turbulence_paths = NULL;
	} /* end if */
}


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
#include <turbulence-ppath.h>

/* local include */
#include <turbulence-ctx-private.h>

typedef enum {
	PROFILE_ALLOW, 
	PROFILE_IF
} TurbulencePPathItemType;

typedef struct _TurbulencePPathItem TurbulencePPathItem;

struct _TurbulencePPathItem {
	/* The type of the profile item path  */
	TurbulencePPathItemType type;
	
	/* support for the profile to be matched by this profile item
	 * path */
	TurbulenceExpr * profile;

	/* optional expression to match a mark that must have a
	 * connection holding the profile */
	char * connmark;
	
	/* optional configuration that allows to configure the number
	   number of channels opened with a particular profile */
	int    max_per_con;

	/* optional expression to match a pre-mark that must have the
	 * connection before accepting the profile. */
	char * preconnmark;

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
	TurbulenceExpr * serverName;

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	TurbulenceExpr * src;

	/* a reference to the list of profile path supported */
	TurbulencePPathItem ** ppath_item;
	
} TurbulencePPathDef;

typedef struct _TurbulencePPathState {
	/* a reference to the profile path selected for the
	 * connection */
	TurbulencePPathDef * path_selected;


	/* turbulence context */
	TurbulenceCtx      * ctx;
} TurbulencePPathState;

struct _TurbulencePPath {
	/* list of profile paths found */
	TurbulencePPathDef ** items;
	
};

TurbulencePPathItem * __turbulence_ppath_get_item (TurbulenceCtx * ctx, axlNode * node)
{
	axlNode             * child;
	int                   iterator;
	TurbulencePPathItem * result = axl_new (TurbulencePPathItem, 1);

	/* get the profile expression */
	result->profile = turbulence_expr_compile (
		/* turbulence context */
		ctx, 
		/* the expression */
		ATTR_VALUE (node, "profile"), 
		/* error message */
		"Failed to get profile expression..");
	if (result->profile == NULL) {
		axl_free (result);
		return NULL;
	} /* end if */

	/* get the connmark flag if defined */
	if (HAS_ATTR (node, "connmark")) {
		result->connmark = axl_strdup (ATTR_VALUE (node, "connmark"));
	}

	/* get the pre-connmark flag if defined */
	if (HAS_ATTR (node, "preconnmark")) {
		result->preconnmark = axl_strdup (ATTR_VALUE (node, "preconnmark"));
	}

	/* get max per con flag */
	if (HAS_ATTR (node, "max-per-conn")) {
		/* get the value and normalize */
		result->max_per_con = atoi (ATTR_VALUE (node, "max-per-conn"));
		if (result->max_per_con < 0)
			result->max_per_con = 0;
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
			result->ppath_item[iterator] = __turbulence_ppath_get_item (ctx, child);
			
			/* next profile path item */
			child = axl_node_get_next (child);
			iterator++;
			
		} /* end if */
	} /* end if */
	
	/* return result parsed */
	return result;
}

#define TURBULENCE_PPATH_STATE "tu::pp:st"

int  __turbulence_ppath_mask_items (TurbulenceCtx        * ctx,
				    TurbulencePPathItem ** ppath_items, 
				    TurbulencePPathState * state, 
				    const char           * uri, 
				    const char           * serverName,
				    int                    channel_num,
				    VortexConnection     * connection,
				    const char           * profile_content)
{
	int                   iterator;
	TurbulencePPathItem * item;
	axlList             * profiles;
	int                   iterator2;
	char                * uri2;
	VortexCtx           * vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	iterator = 0;
	while (ppath_items[iterator]) {
		/* item from the profile path */
		item = ppath_items[iterator];

		/* check the profile uri */
		if (! turbulence_expr_match (item->profile, uri)) {
			/* profile doesn't match, go to the next */
			iterator++;
			continue;
		} /* end if */

		/* now check its optional pre-mark */
		if (item->preconnmark != NULL) {
			if (! vortex_connection_get_data (connection, item->preconnmark)) {
				/* the mark doesn't match the connection */
				iterator++;
				continue; 
			} /* end if */
		} /* end if */

		/* check item count */
		if (item->max_per_con > 0) {
			/* check if the profile was used more than the
			 * value configured */

			if (vortex_connection_get_channel_count (connection, (const char *) item->profile) >= item->max_per_con) {
				/* too much channels opened for the same uri */
				iterator++;
				continue;
			} /* end if */
		} /* end if */

		/* profile path matched! */

		/* check if the channel num is defined */
		if (channel_num > 0  && state->path_selected->serverName != NULL) {

			/* check the serverName value provided against
			 * the configuration */
			if (! turbulence_expr_match (state->path_selected->serverName, serverName ? serverName : "")) {
				error ("serverName='%s' doesn't match profile path conf", serverName ? serverName : "");
				/* filter the channel creation because
				 * the serverName provided doesn't
				 * match */
				return axl_true;
			} /* end if */
		} /* end if */

		/* profile properly matched, including the serverName */
		return axl_false;
	} /* end if */

	/* now check for second level profile path configurations,
	 * based on <if-success> */
	iterator = 0;
	while (ppath_items[iterator]) {
		/* item from the profile path */
		item = ppath_items[iterator];

		/* check if the profile path item is an IF
		 * expression. In such case, check if the profile
		 * provided by the if-expression is already running on
		 * the connection. If the connection have the profile,
		 * check all <allow> and <if-success> nodes inside. */
		if (item->type == PROFILE_IF) {
			/* try to find a profile that matches the expression found */
			profiles = vortex_profiles_get_actual_list_ref (vortex_ctx);
			iterator2 = 0;
			while (iterator2 < axl_list_length (profiles)) {

				/* get the uri value */
				uri2 = axl_list_get_nth (profiles, iterator2);
				
				/* try to match the profile expression against
				   a concrete profile value */
				if ( turbulence_expr_match (item->profile, uri2)) {

					/* found, now check if the profile is running on the
					 * conection */
					if (vortex_connection_get_channel_by_uri (connection, uri2)) {

						/* now check its optional mark */
						if (item->connmark != NULL) {
							if (! vortex_connection_get_data (connection, item->connmark)) {
								/* the mark doesn't match the connection */
								iterator2++;
								continue; 
							} /* end if */
						} /* end if */

						/* check if the profile provided is found in the allow
						 * configuration */
						if (! __turbulence_ppath_mask_items (ctx, 
										     item->ppath_item, 
										     state, uri, serverName, channel_num, connection, profile_content)) {
							/* profile allowed, do not filter */
							return axl_false;
						} /* end if */

					} /* end if */

				} /* end if */

				/* go to the next item */
				iterator2++;

			} /* end while */

		} /* end if */

		/* next item to process */
		iterator++;
	} /* end if */

	return axl_true;
}

/** 
 * @internal Mask function that allows to control how profiles are
 * handled and sequenced by the client according to the state of the
 * connection (profiles already accepted, etc).
 */
int  __turbulence_ppath_mask (VortexConnection  * connection, 
			      int                 channel_num,
			      const char        * uri,
			      const char        * profile_content,
			      const char        * serverName,
			      char             ** error_msg,
			      axlPointer         user_data)
{
	/* get a reference to the turbulence profile path state */
	TurbulencePPathState  * state  = user_data;
	TurbulenceCtx         * ctx    = state->ctx;

	/* check if the profile provided is found in the <allow> or
	 * <if-success> configuration */
	if (! __turbulence_ppath_mask_items (ctx, 
					     state->path_selected->ppath_item, 
					     state, uri, serverName, channel_num, connection, profile_content)) {

		/* only drop a message if the channel number have a
		 * valid value. Profile mask is also executed at
		 * greetings phase and channel_num is equal to -1. In
		 * this case we can't say the channel have been
		 * accepted */
		if (channel_num > 0) {
			msg ("profile: %s accepted (ppath: \"%s\" conn id: %d [%s:%s])", 
			     uri, state->path_selected->path_name, 
			     vortex_connection_get_id (connection), 
			     vortex_connection_get_host (connection),
			     vortex_connection_get_port (connection));
			
			/* report access */
			access ("profile: %s accepted (ppath: \"%s\" conn id: %d [%s:%s])", 
				uri, state->path_selected->path_name, 
				vortex_connection_get_id (connection), 
				vortex_connection_get_host (connection),
				vortex_connection_get_port (connection));
		}
		
		/* profile allowed, do not filter */
		return axl_false;
	} /* end if */

	/* drop an error message if a definitive channel request was
	 * received */
	if (channel_num > 0) {
		error ("profile: %s not accepted (ppath: \"%s\" conn id: %d [%s:%s])", 
		       uri, state->path_selected->path_name, 
		       vortex_connection_get_id (connection), 
		       vortex_connection_get_host (connection),
		       vortex_connection_get_port (connection));
		if (error_msg) {
			(*error_msg) = axl_strdup_printf (
				"PROFILE PATH configuration denies creating the channel with the profile requested: %s not accepted (ppath: \"%s\" conn id: %d [%s:%s])", 
				uri, state->path_selected->path_name, 
				vortex_connection_get_id (connection), 
				vortex_connection_get_host (connection),
				vortex_connection_get_port (connection));
		} /* end if */
	} /* end if */
	

	/* filter any other option */
	return axl_true;
}

/** 
 * @internal Server init handler that allows to check the connectio
 * source and select the appropiate profile path to be used for the
 * connection. It also sets the required handler to enforce profile
 * path policy.
 *
 * In the case a profile path definition is not found, the handler
 * just denies the connection.
 */
int  __turbulence_ppath_handle_connection (VortexConnection * connection, axlPointer data)
{
	/* get turbulence context */
	TurbulencePPathState * state;
	TurbulencePPathDef   * def = NULL;
	TurbulenceCtx        * ctx;
	int                    iterator;
	const char           * src;

	/* get the current context (TurbulenceCtx) */
	ctx = data;

	/* try to find a profile path that could match with the
	 * provided source */
	iterator = 0;
	src      = vortex_connection_get_host (connection);
	while (ctx->paths->items[iterator] != NULL) {
		/* get the profile path def */
		def = ctx->paths->items[iterator];
		msg ("checking profile path def: %s", def->path_name ? def->path_name : "(no path name defined)");

		/* try to match the src expression against the connection value */
		if (turbulence_expr_match (def->src, src)) {
			/* match found */
			msg ("profile path found, setting default state: %s, connection id=%d, src=%s", 
			     def->path_name ? def->path_name : "(no path name defined)",
			     vortex_connection_get_id (connection), src);
			break;
		}
		
		/* next profile path definition */
		iterator++;
		def = NULL;

	} /* end while */
	
	if (def == NULL) {
		/* no profile path def was found, rejecting
		 * connection */
		error ("no profile path def match, rejecting connection: id=%d, src=%s", 
		       vortex_connection_get_id (connection), src);
		return axl_false;
	} /* end if */

	/* create and store */
	state                = axl_new (TurbulencePPathState, 1);
	state->path_selected = def;
	state->ctx           = ctx;
	vortex_connection_set_data_full (connection, 
					 /* the key and its associated value */
					 TURBULENCE_PPATH_STATE, state,
					 /* destroy functions */
					 NULL, axl_free);
	
	/* now configure the profile path mask to handle how channels
	 * and profiles are accepted */
	vortex_connection_set_profile_mask (connection, __turbulence_ppath_mask, state);
	
	return axl_true;
}

/** 
 * @internal Prepares the runtime execution to provide profile path
 * support according to the current configuration.
 * 
 */
int  turbulence_ppath_init (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	axlNode            * node; 
	axlNode            * pdef;
	TurbulencePPathDef * definition;
	int                  iterator;
	int                  iterator2;
	VortexCtx          * vortex_ctx = turbulence_ctx_get_vortex_ctx (ctx);

	/* check turbulence context received */
	v_return_val_if_fail (ctx, axl_false);
	
	/* parse all profile path configurations */
	pdef = axl_doc_get (turbulence_config_get (ctx), "/turbulence/profile-path-configuration/path-def");
	if (pdef == NULL) {
		error ("No profile path configuration was found, you must set at least one profile path.");
		return axl_false;
	} /* end if */
	
	/* get the parent node */
	node = axl_node_get_parent (pdef);
	
	/* create the turbulence ppath */
	ctx->paths        = axl_new (TurbulencePPath, 1);
	ctx->paths->items = axl_new (TurbulencePPathDef *, axl_node_get_child_num (node) + 1);

	/* now parse each profile path def found */
	iterator = 0;
	while (pdef != NULL) {

		/* get the reference to the profile path */
		ctx->paths->items[iterator] = axl_new (TurbulencePPathDef, 1);
		definition                  = ctx->paths->items[iterator];

		/* catch all data from the profile path def header */
		if (HAS_ATTR (pdef, "path-name")) {
			/* catch the ppath name */
			definition->path_name = axl_strdup (ATTR_VALUE (pdef, "path-name"));
		} /* end if */

		/* catch server name match */
		if (HAS_ATTR (pdef, "server-name")) {
			definition->serverName = turbulence_expr_compile (ctx, 
									  ATTR_VALUE (pdef, "server-name"),
									  "Failed to parse \"server-name\" expression at profile def");
		} /* end if HAS_ATTR (pdef, "server-name")) */

		if (HAS_ATTR (pdef, "src")) {
			definition->src = turbulence_expr_compile (ctx,
								   ATTR_VALUE (pdef, "src"),
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
			definition->ppath_item[iterator2] = __turbulence_ppath_get_item (ctx, node);
			
			/* next profile path item */
			node = axl_node_get_next (node);
			iterator2++;
			
		} /* end if */
		
		/* get next profile path def */
		iterator++;
		pdef = axl_node_get_next (pdef);
	} /* end while */

	/* install server connection accepted */
	vortex_listener_set_on_connection_accepted (vortex_ctx, __turbulence_ppath_handle_connection, ctx);
	
	msg ("profile path definition ok..");

	/* return ok code */
	return axl_true;
}

void __turbulence_ppath_free_item (TurbulencePPathItem * item)
{
	int iterator;

	/* free profile expression */
	turbulence_expr_free (item->profile);

	/* free connmark and pre-connmark */
	axl_free (item->connmark);
	axl_free (item->preconnmark);

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
void turbulence_ppath_cleanup (TurbulenceCtx * ctx)
{
	int iterator;
	int iterator2;
	TurbulencePPathDef * def;

	/* terminate profile paths */
	if (ctx->paths != NULL) {

		/* for each profile path item iterator */
		iterator = 0;
		while (ctx->paths->items[iterator] != NULL) {
			/* get the definition */
			def       = ctx->paths->items[iterator];

			/* free profile path name definition */
			axl_free (def->path_name);
			turbulence_expr_free (def->serverName);
			turbulence_expr_free (def->src);

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
		axl_free (ctx->paths->items);
		axl_free (ctx->paths);
		ctx->paths = NULL;
	} /* end if */
}


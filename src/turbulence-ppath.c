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
#include <turbulence-ppath.h>

/* local include */
#include <turbulence-ctx-private.h>

#if defined (ENABLE_PCRE_SUPPORT)
#include <pcre.h>
typedef struct _TurbuleceExpression {
	pcre * expr;
	bool   negative;
}TurbulenceExpression;

#endif

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
TurbulenceExpression
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
	expr * serverName;

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	expr * src;

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

/** 
 * @internal Function used to check if the string provided have
 * content that must be revised to help its clarity.
 */
bool      __turbulence_ppath_has_escapable_chars        (const char * expression,
							 int          expression_size,
							 int        * added_size)
{
	int      iterator = 0;
	bool     result   = false;

	/* reset additional size value */
	*added_size = 0;

	/* calculate the content size */
	expression_size = strlen (expression);

	/* iterate over all content defined */
	while (iterator < expression_size) {

		/* check for .* */
		if (expression [iterator] == '*' && expression [iterator - 1] != '.' ) {
			result = true;
			(*added_size) += 1;
		}

		/* check for \/ */
		if (expression [iterator] == '/' && expression [iterator - 1] != '\\' ) {
			result = true;
			(*added_size) += 1;
		}

		/* update the iterator */
		iterator++;
	} /* end if */

	/* return results */
	return result;
} /* end __turbulence_ppath_has_escapable_chars */

/** 
 * @internal Support function which used information provided by
 * previous function to expand the string providing a perl regular
 * expression compatible if used / and *.
 */
char * __turbulence_ppath_copy_and_escape (const char * expression, 
					   int          expression_size, 
					   int          additional_size)
{
	int    iterator  = 0;
	int    iterator2 = 0;
	char * result;
	axl_return_val_if_fail (expression, false);

	/* allocate the memory to be returned */
	result = axl_new (char, expression_size + additional_size + 1);

	/* iterate over all expression defined */
	while (iterator2 < expression_size) {
		if (iterator2 > 0) {
			/* check for .* */
			if (expression [iterator2] == '*' && expression [iterator2 - 1] != '.') {
				memcpy (result + iterator, ".*", 2);
				iterator += 2;
				iterator2++;
				continue;
			}
			
			/* check for .* */
			if (expression [iterator2] == '/' && expression [iterator2 - 1] != '\\') {
				memcpy (result + iterator, "\\/", 2);
				iterator += 2;
				iterator2++;
				continue;
			}
		} /* end if */

		/* copy value received because it is not an escape
		 * sequence */
		memcpy (result + iterator, expression + iterator2, 1);

		/* update the iterator */
		iterator++;
		iterator2++;
	} /* end while */

	/* return results */
	return result;
} /* end __turbulence_ppath_copy_and_escape */

/* check if the expression is negative */
const char * __turbulence_ppath_check_negative_expr (const char * expression, bool * negative)
{
	int iterator;
	int length;

	/* do not oper if null is received */
	if (expression == NULL)
		return expression;

	/* check if the negative expression is found */
	iterator = 0;
	length   = strlen (expression);

	/* skip white spaces */
	while (iterator < length) {
		if (expression [iterator] == ' ')
			iterator++;
		else
			break;
	} /* end while */

	/* check negative expression */
	if (expression [iterator] == '!' || 
	    (((iterator + 2) < length) && expression[iterator] == 'n' && expression[iterator + 1] == 'o' && expression[iterator + 2] == 't')) {
		/* negative expression found, return the updated
		 * reference */
		*negative = true;
		if (expression[iterator] == 'n')
			return (expression  + iterator + 4);
		else
			return (expression  + iterator + 2);
	} /* end if */

	/* return the expression as is */
	return expression;
}

/**
 * @internal Support function to parse the expression provided or to
 * just copy the string if turbulence wasn't built without pcre
 * support.
 */
axlPointer __turbulence_ppath_compile_expr (TurbulenceCtx * ctx, const char * expression, const char * error_msg)
{
#if defined(ENABLE_PCRE_SUPPORT)
	TurbulenceExpression * expr;
	const char           * error;
	int                    erroroffset;
	bool                   dealloc = false;
	int                    additional_size;

	/* create the turbulence expression node */
	expr = axl_new (TurbulenceExpression, 1);

	/* check if the expression is negative */
	msg ("checking negative expression: %s", expression);
	expression = __turbulence_ppath_check_negative_expr (expression, &expr->negative);
	if (expr->negative) {
		msg ("  found, updated expression to: not %s", expression);
	}
	
	/* do some regular expression support to avoid making it
	 * painful */
	if (__turbulence_ppath_has_escapable_chars (expression, strlen (expression), &additional_size)) {
		/* expand all * and / values which are not preceded as
		 * .* or \/ */
		msg ("NOTE: expanding expression: %s..", expression);
		expression = __turbulence_ppath_copy_and_escape (expression, strlen (expression), additional_size);
		msg ("      to: %s..", expression);
		dealloc    = true;
	} /* end if */
	
	/* compile expression */
	expr->expr = pcre_compile (
		/* the pattern to compile */
		expression, 
		/* no flags */
		0,
		/* provide a reference to the error reporting */
		&error, &erroroffset, NULL);

	if (expr->expr == NULL) {
		/* failed to parse server name matching */
		error ("%s: %s, error: %s, at: %d",
		       error_msg, expression, error, erroroffset);

		/* free expr */
		axl_free (expr);

		/* check and dealloc */
		if (dealloc)
			axl_free ((char*) expression);
		return NULL;
	} /* end if */

	/* check and dealloc */
	if (dealloc)
		axl_free ((char *) expression);

	/* return expression */
	return expr;
#else
	/* just copy the expression */
	return axl_strdup (expression);
#endif

} /* end __turbulence_ppath_compile_expr */

/** 
 * @internal Implementation that free the received expression
 * according to the compilation process.
 */
void __turbulence_ppath_free_expr (axlPointer _expr)
{

#if defined (ENABLE_PCRE_SUPPORT)
	TurbulenceExpression * expr = _expr;
#endif
	if (_expr == NULL)
		return;
#if defined(ENABLE_PCRE_SUPPORT)
	pcre_free (expr->expr);
	axl_free (expr);
#else
	axl_free (_expr);
#endif
	return;
}

/** 
 * @internal Function that allows perform matching against the
 * expression, using the subject received. According to the
 * compilation process, this operation will be performed against the
 * pcre library or a simple string match.
 */
bool __turbulence_ppath_match_expr (axlPointer _expr, const char * subject)
{
#if defined (ENABLE_PCRE_SUPPORT)
	TurbulenceExpression * expr = _expr;
#endif

	/* return false if either values received are null */
	if (subject == NULL || _expr == NULL)
		return false;

#if defined(ENABLE_PCRE_SUPPORT)
	/* check against the pcre expression */
	if (expr->negative) {
		return ! (pcre_exec (expr->expr, NULL, subject, strlen (subject), 0, 0, NULL, 0) >= 0);
	} else {
		return pcre_exec (expr->expr, NULL, subject, strlen (subject), 0, 0, NULL, 0) >= 0;
	}
#else
	return axl_cmp (_expr, subject);
#endif
}

TurbulencePPathItem * __turbulence_ppath_get_item (TurbulenceCtx * ctx, axlNode * node)
{
	axlNode             * child;
	int                   iterator;
	TurbulencePPathItem * result = axl_new (TurbulencePPathItem, 1);

	/* get the profile expression */
	result->profile = __turbulence_ppath_compile_expr (
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

bool __turbulence_ppath_mask_items (TurbulenceCtx        * ctx,
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

	iterator = 0;
	while (ppath_items[iterator]) {
		/* item from the profile path */
		item = ppath_items[iterator];

		/* check the profile uri */
		if (! __turbulence_ppath_match_expr (item->profile, uri)) {
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
			if (! __turbulence_ppath_match_expr (state->path_selected->serverName, serverName ? serverName : "")) {
				error ("serverName='%s' doesn't match profile path conf", serverName ? serverName : "");
				/* filter the channel creation because
				 * the serverName provided doesn't
				 * match */
				return true;
			} /* end if */
		} /* end if */

		/* profile properly matched, including the serverName */
		return false;
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
			profiles = vortex_profiles_get_actual_list_ref ();
			iterator2 = 0;
			while (iterator2 < axl_list_length (profiles)) {

				/* get the uri value */
				uri2 = axl_list_get_nth (profiles, iterator2);
				
				/* try to match the profile expression against
				   a concrete profile value */
				if ( __turbulence_ppath_match_expr (item->profile, uri2)) {

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
							return false;
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

	return true;
}

/** 
 * @internal Mask function that allows to control how profiles are
 * handled and sequenced by the client according to the state of the
 * connection (profiles already accepted, etc).
 */
bool __turbulence_ppath_mask (VortexConnection * connection, 
			      int                channel_num,
			      const char       * uri,
			      const char       * profile_content,
			      const char       * serverName,
			      axlPointer         user_data)
{
	/* get a reference to the turbulence profile path state */
	TurbulencePPathState  * state = user_data;
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
		return false;
	} /* end if */

	/* drop an error message if a definitive channel request was
	 * received */
	if (channel_num > 0) 
		error ("profile: %s not accepted (ppath: \"%s\" conn id: %d [%s:%s])", 
		       uri, state->path_selected->path_name, 
		       vortex_connection_get_id (connection), 
		       vortex_connection_get_host (connection),
		       vortex_connection_get_port (connection));

	/* filter any other option */
	return true;
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
bool __turbulence_ppath_handle_connection (VortexConnection * connection, axlPointer data)
{
	/* get turbulence context */
	TurbulencePPathState * state;
	TurbulencePPathDef   * def = NULL;
	TurbulenceCtx        * ctx;
	int                    iterator;
	const char           * src;

	/* get the current context (TurbulenceCtx) */
	ctx = data;

	fprintf (stderr, "Profile path applied on context: %p", ctx);
	fflush (stderr);
	

	/* try to find a profile path that could match with the
	 * provided source */
	iterator = 0;
	src      = vortex_connection_get_host (connection);
	while (ctx->paths->items[iterator] != NULL) {
		/* get the profile path def */
		def = ctx->paths->items[iterator];
		msg ("checking profile path def: %s", def->path_name ? def->path_name : "(no path name defined)");

		/* try to match the src expression against the connection value */
		if (__turbulence_ppath_match_expr (def->src, src)) {
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
		return false;
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
	
	return true;
}

/** 
 * @internal Prepares the runtime execution to provide profile path
 * support according to the current configuration.
 * 
 */
bool turbulence_ppath_init (TurbulenceCtx * ctx)
{
	/* get turbulence context */
	axlNode            * node; 
	axlNode            * pdef;
	TurbulencePPathDef * definition;
	int                  iterator;
	int                  iterator2;

	/* check turbulence context received */
	v_return_val_if_fail (ctx, false);
	
	/* parse all profile path configurations */
	pdef = axl_doc_get (turbulence_config_get (ctx), "/turbulence/profile-path-configuration/path-def");
	if (pdef == NULL) {
		error ("No profile path configuration was found, you must set at least one profile path.");
		return false;
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
			definition->serverName = __turbulence_ppath_compile_expr (ctx, 
										  ATTR_VALUE (pdef, "server-name"),
										  "Failed to parse \"server-name\" expression at profile def");
		} /* end if HAS_ATTR (pdef, "server-name")) */

		if (HAS_ATTR (pdef, "src")) {
			definition->src = __turbulence_ppath_compile_expr (ctx,
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
	vortex_listener_set_on_connection_accepted (__turbulence_ppath_handle_connection, ctx);
	
	msg ("profile path definition ok..");

	/* return ok code */
	return true;
}

void __turbulence_ppath_free_item (TurbulencePPathItem * item)
{
	int iterator;

	/* free profile expression */
	__turbulence_ppath_free_expr (item->profile);

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
		axl_free (ctx->paths->items);
		axl_free (ctx->paths);
		ctx->paths = NULL;
	} /* end if */
}


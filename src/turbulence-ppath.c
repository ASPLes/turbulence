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

struct _TurbulencePPathDef {
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

	/* destination filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	TurbulenceExpr * dst;

	/* a reference to the list of profile path supported */
	TurbulencePPathItem ** ppath_item;

#if defined(AXL_OS_UNIX)
	/* user id to that must be used to run the process */
	int  user_id;

	/* group id that must be used to run the process */
	int  group_id;
#endif

	/* allows to control if turbulence must run the connection
	 * received in the context of a profile path in a separate
	 * process */
	axl_bool separate;

	/* allows to change working directory to the provided value */
	const char * chroot;	

	/* allows to configure a working directory associated to the
	 * profile path. */
	const char * work_dir;
};

typedef struct _TurbulencePPathState {
	/* a reference to the profile path selected for the
	 * connection */
	TurbulencePPathDef * path_selected;

	/* requested serverName found at the profile path selection */
	char               * requested_serverName;

	/* turbulence context */
	TurbulenceCtx      * ctx;
} TurbulencePPathState;

void __turbulence_ppath_state_free (axlPointer _state) {
	TurbulencePPathState * state = _state;

	if (state == NULL)
		return;
	axl_free (state->requested_serverName);
	axl_free (state);
	return;
}

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
axl_bool  __turbulence_ppath_mask (VortexConnection  * connection, 
				   int                 channel_num,
				   const char        * uri,
				   const char        * profile_content,
				   VortexEncoding      encoding,
				   const char        * serverName,
				   VortexFrame       * frame,
				   char             ** error_msg,
				   axlPointer         user_data)
{
	/* get a reference to the turbulence profile path state */
	TurbulencePPathState  * state  = user_data;
	TurbulenceCtx         * ctx    = state->ctx;

	if (state == NULL || state->path_selected == NULL) {
		error ("No profile path selected, deny: %s (conn id: %d [%s:%s])", 
				uri, 
				vortex_connection_get_id (connection), 
				vortex_connection_get_host (connection),
				vortex_connection_get_port (connection));
		/* filter, no profile path selected */
		return axl_true;
	} /* end if */

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
 * @internal Mask function that is used to provide access to still
 * unclassified BEEP connections to profiles available at the root
 * level on existing profile path configurations. When this mask is
 * called means that not all profile path rules are ip based so it is
 * required further steps in the BEEP negotiation to let the
 * connection BEEP peer to provide a serverName that will finally (or
 * not) select an appropriate profile path.
 *
 * This function has a relation with __turbulence_ppath_mask in the
 * fact that the former is called when it is clear the profile path
 * selected (due to address rules). When this function (mask_temporal)
 * selects an appropriate profile mask, it is then configured
 * __turbulence_ppath_mask to be called always.
 *
 */
axl_bool  __turbulence_ppath_mask_temporal   (VortexConnection  * connection, 
					      int                 channel_num,
					      const char        * uri,
					      const char        * profile_content,
					      VortexEncoding      encoding,
					      const char        * serverName,
					      VortexFrame       * frame,
					      char             ** error_msg,
					      axlPointer          user_data)
{
	TurbulencePPathState * state;
	TurbulenceCtx        * ctx;
	
	/* get current state */
	state = vortex_connection_get_data (connection, TURBULENCE_PPATH_STATE);
	ctx   = state->ctx;

	msg ("Called to temporal profile mask on %s phase, for connection id=%d", 
	     channel_num == -1 ? "greetings" : "channel creation", vortex_connection_get_id (connection));
	
	/* check if the state has a profile path selected */
	if (state->path_selected == NULL) {
		/* no profile path selected, check if we are in greetings phase */
		if (channel_num == -1) 
			return axl_true; /* filter profile */

		/* call to select a profile path with the received
		   serverName and signaling we are NOT in on connect phase */
		if (! __turbulence_ppath_select (state->ctx, connection, channel_num, uri, profile_content, encoding, serverName, frame, axl_false)) {
			error ("channel creation for profile %s was filtered since no profile path was found..", uri);
			return axl_true; /* filter channel creation */
		} /* end if */

		/* check if a profile path was selected to close the
		   connection if not */
		if (state->path_selected == NULL) {
			error ("Unable to accept connection, no profile path matches after first channel start for uri=%s: conn id: %d [%s:%s])", 
			       uri,
			       vortex_connection_get_id (connection), 
			       vortex_connection_get_host (connection),
			       vortex_connection_get_port (connection));
			vortex_connection_shutdown (connection);
			return axl_false;
		} /* end if */

		/* check if the connection will be handled by a child
		 * proces */
		if (state->path_selected->separate) {
			/* flag the connection to skip futher handling
			 * letting the child process to finish start
			 * handle */
			vortex_connection_set_data (connection, VORTEX_CONNECTION_SKIP_HANDLING, INT_TO_PTR (axl_true));
			return axl_true; /* return true, to filter at
					  * the parent and to skip
					  * parent handling */
		} /* end if */

	} /* end if */

	/* reached this point we have the path seletected so call to
	   base function */
	return __turbulence_ppath_mask (connection, channel_num, uri, profile_content, encoding, serverName, frame, error_msg, user_data);
}

axl_bool __turbulence_ppath_handle_connection_match_src (VortexConnection * connection, 
							 TurbulenceExpr   * expr, 
							 const char       * src)
{
	/* by default, it no expression is found, it means this match
	 * pattern must not block the connection  */
	if (expr == NULL)
		return axl_true;

	/* try to match the src expression against the connection value */
	if (turbulence_expr_match (expr, src)) {
		return axl_true;
	} /* end if */
	
	return axl_false;
}

axl_bool __turbulence_ppath_handle_connection_match_dst (VortexConnection * connection, 
							 TurbulenceExpr   * expr, 
							 const char       * dst)
{
	/* by default, it no expression is found, it means this match
	 * pattern must not block the connection  */
	if (expr == NULL)
		return axl_true;

	/* try to match the src expression against the connection value */
	if (turbulence_expr_match (expr, dst)) {
		return axl_true;
	} /* end if */
	
	return axl_false;
}

/** 
 * @internal Function that allows to select a profile path for a
 * connection. The variable on_connect signals if the connection
 * notified is on server accept or because client greetings was
 * received.
 *
 * @return The function returns axl_false to signal that the
 * connection not accepted due to profile path configuration. The
 * caller must deny connection operation according to the connection
 * stage, otherwise axl_true is returned either because the profile
 * path was configured or because it will be configured on next calls
 * to __turbulence_ppath_select
 */
axl_bool __turbulence_ppath_select (TurbulenceCtx      * ctx, 
				    VortexConnection   * connection, 
				    int                  channel_num,
				    const char         * uri,
				    const char         * profile_content,
				    VortexEncoding       encoding,
				    /* value requested through x-serverName (serverName) feature. */
				    const char         * serverName, 
				    VortexFrame        * frame,
				    axl_bool             on_connect)
{
	/* get turbulence context */
	TurbulencePPathState * state;
	TurbulencePPathDef   * def = NULL;
	int                    iterator;
	const char           * src;
	const char           * dst;
	axl_bool               src_status;
	axl_bool               dst_status;
	axl_bool               serverName_status;

	if (on_connect) {
		/* called to select profile path at connection time:
		   still BEEP listener greetings wasn't sent so we can
		   only select if all profile path references to src=
		   and dst= */
		if (! ctx->all_rules_address_based)  {
			/* configure a profile mask to select an appropriate ppath state in the next channel
			   start request where the remote BEEP peer has a chance to select a serverName value */
			state                = axl_new (TurbulencePPathState, 1);
			state->path_selected = NULL; /* still no profile path selected */
			state->ctx           = ctx;
			vortex_connection_set_data_full (connection, 
							 /* the key and its associated value */
							 TURBULENCE_PPATH_STATE, state,
							 /* destroy functions */
							 NULL, __turbulence_ppath_state_free);
			vortex_connection_set_profile_mask (connection, __turbulence_ppath_mask_temporal, state);

			return axl_true; /* signal no profile path still selected */
		} /* end if */
	} /* end if */

	/* try to find a profile path that match with the provided
	 * source */
	iterator = 0;
	src      = vortex_connection_get_host (connection);
	dst      = vortex_connection_get_local_addr (connection);
	while (ctx->paths->items[iterator] != NULL) {
		/* get the profile path def */
		def = ctx->paths->items[iterator];
		msg ("checking profile path def: %s", def->path_name ? def->path_name : "(no path name defined)");

		/* get src status */
		src_status        = __turbulence_ppath_handle_connection_match_src (connection, def->src, src);

		/* get dst status */
		dst_status        = __turbulence_ppath_handle_connection_match_src (connection, def->dst, dst);

		/* get serverName status */
		serverName_status = __turbulence_ppath_handle_connection_match_src (connection, def->serverName, serverName);

		/* match found */
		if (src_status && dst_status && serverName_status) {
			msg ("profile path found, setting default state: %s, connection id=%d, src=%s local_addr=%s serverName=%s ", 
			     def->path_name ? def->path_name : "(no path name defined)",
			     vortex_connection_get_id (connection), src, dst, serverName ? serverName : "");
			break;
		} else {
			/* show profile path not mached */
			msg2 ("profile path do not match: %s, for connection id=%d, src=%s local_addr=%s serverName=%s ", 
			      def->path_name ? def->path_name : "(no path name defined)",
			      vortex_connection_get_id (connection), src, dst, serverName ? serverName : "");
		} /* end if */

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

	/* check if this function was called to select a path with an
	   state created */
	state                = vortex_connection_get_data (connection, TURBULENCE_PPATH_STATE);
	if (state != NULL) {
		state->path_selected = def;
	} else {
		/* create and store */
		state                = axl_new (TurbulencePPathState, 1);
		state->path_selected = def;
		state->ctx           = ctx;
		vortex_connection_set_data_full (connection, 
						 /* the key and its associated value */
						 TURBULENCE_PPATH_STATE, state,
						 /* destroy functions */
						 NULL, __turbulence_ppath_state_free);
		
		/* now configure the profile path mask to handle how channels
		 * and profiles are accepted */
		vortex_connection_set_profile_mask (connection, __turbulence_ppath_mask, state);
	} /* end if */

	/* profile path selected but we have no way to configure the
	 * serverName to be used on this connection until the first
	 * channel is accepted (with the serverName configured). So
	 * the following sets the requested serverName so modules
	 * notified through profile path selected and react according
	 * to this value. */
	if (serverName) {
		msg ("Setting requested serverName=%s but still first opened channel is required", serverName);
		state->requested_serverName = axl_strdup (serverName);
	} /* end if */
		
	/* check for process separation and apply operation here */
	if (def->separate) {
		/* call to create process */
		turbulence_process_create_child (ctx, connection, def, 
						 /* signal to handle start request reply */
						 ! on_connect, 
						 channel_num,
						 uri, profile_content, encoding, serverName, 
						 frame);
		msg ("finished turbulence_process_create_child..");
	} else {

		/* notify profile path selected in the case no child
		   is created because turbulence_process_create_child
		   already do it */
		if (! turbulence_module_notify (ctx, TBC_PPATH_SELECTED_HANDLER, def, connection, NULL)) {
			CLEAN_START(ctx); /* check to terminate child if clean start is defined */
		}
	} /* end if */
	
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
axl_bool  __turbulence_ppath_handle_connection_on_connect (VortexConnection * connection, axlPointer data)
{
	/* call to select a profile path: serverName = NULL ("") && on_connect = axl_true */
	return __turbulence_ppath_select ((TurbulenceCtx *) data, connection, -1, NULL, NULL, -1, "", NULL, axl_true);
}


/** 
 * @internal Helper for __turbulence_ppath_get_server_name_feature.
 */
char * __turbulence_ppath_get_server_name_feature_aux (const char * features)
{
	int    last     = 0;
	int    iterator = 0;
	char * result;

	/* check last position */
	while (iterator < features[last] && features[last] != 0 && features[last] != ' ')
		last++;

	/* check for empty results */
	if (last == iterator)
		return NULL;
	
	/* check we have found last position */
	if (features[last] == 0 || features[last] == ' ') {
		result = axl_new (char, last - iterator + 1);
		memcpy (result, features, last - iterator);
		return result;
	}
	return NULL;
}

/** 
 * @internal Function used to get the serverName reported by
 * x-serverName feature (if found). The function returns NULL if
 * nothing is found.
 */
char * __turbulence_ppath_get_server_name_feature (const char * features)
{
	int iterator = 0;

	/* check for empty features */
	if (features == NULL)
		return NULL;

	while (iterator < strlen (features)) {

		if (axl_memcmp (features + iterator, "x-serverName:", 13)) 
			return __turbulence_ppath_get_server_name_feature_aux (features + iterator + 13);
		if (axl_memcmp (features + iterator, "serverName:", 11)) 
			return __turbulence_ppath_get_server_name_feature_aux (features + iterator + 11);
		if (axl_memcmp (features + iterator, "x-serverName=", 13)) 
			return __turbulence_ppath_get_server_name_feature_aux (features + iterator + 13);
		if (axl_memcmp (features + iterator, "serverName=", 11)) 
			return __turbulence_ppath_get_server_name_feature_aux (features + iterator + 11);

		/* next position */
		iterator++;
	}
	return NULL;
}

/** 
 * @internal Handler called to configure/reconfigure profile path to
 * be applied to this connection. This handler is called after the
 * connection was accepted and once the client greetings was received.
 *
 * See also __turbulence_ppath_handle_connection_on_connect which also
 * configures profile path but before this function, that is, once the
 * connection is received.
 */
int __turbulence_ppath_handle_connection_on_greetings (VortexCtx               * vortex_ctx,
						       VortexConnection        * connection,
						       VortexConnection       ** new_conn,
						       VortexConnectionStage     state,
						       axlPointer                user_data)
{
	/* for now return ok */
	return 0;
}


/** 
 * @internal Checks the user id value (or group id value if
 * check_user_id == axl_false) to store to user the user_id/group_id
 * that will be used for the process serving BEEP.
 */
void __turbulence_ppath_check_user (TurbulenceCtx      * ctx,
				    TurbulencePPathDef * pdef, 
				    const char         * value, 
				    axl_bool             check_user_id) 
{
	if (check_user_id) {
		/* store user id */
		pdef->user_id =  turbulence_get_system_id (ctx, value, check_user_id);
	} else {
		/* store group */
		pdef->group_id = turbulence_get_system_id (ctx, value, check_user_id);
	} /* end if */
	return;
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

	/* flag profile path rules as only ip based and change this
	   value as long as we read rules */
	ctx->all_rules_address_based = axl_true;

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

		if (HAS_ATTR (pdef, "dst")) {
			definition->dst = turbulence_expr_compile (ctx,
								   ATTR_VALUE (pdef, "dst"),
								   "Failed to parse \"src\" expression at profile def");
		} /* end if (HAS_ATTR (pdef, "dst")) */

		/* ensure all rules we have are address based making
		   posible to apply profile path policy on server
		   accept handler rather waiting client greetings
		   reception */
		if (ctx->all_rules_address_based) {
			/* if has server-Name defined and its value is
			   different .* (which means all serverName
			   allowed including empty value). The following signals all rules are address based if: 
			   - It has serverName defined and
			   - It has a value different from .* and
			   - the rule having serverName configuration have no address match 
			*/

			if ((HAS_ATTR (pdef, "server-name")) && 
			    (! HAS_ATTR_VALUE (pdef, "server-name", ".*")) && 
			    (HAS_ATTR (pdef, "src") || HAS_ATTR (pdef, "dst"))) {
				ctx->all_rules_address_based = axl_false;
			} /* end if */
		} /* end if */

#if defined(AXL_OS_UNIX)
		/* set default user id and group id */
		definition->user_id  = -1;
		definition->group_id = -1;
#endif
		/* get run as user */
		if (HAS_ATTR (pdef, "run-as-user")) {
			/* check value: user-id */
			__turbulence_ppath_check_user (ctx, definition, ATTR_VALUE (pdef, "run-as-user"), axl_true);

			/* get run as group id: only check this value
			 * in the case user-id is configured. It is
			 * not allowed to only set group-id */
			if (HAS_ATTR (pdef, "run-as-group")) {
				/* check value: user-id */
				__turbulence_ppath_check_user (ctx, definition, ATTR_VALUE (pdef, "run-as-group"), axl_false);
			} /* end if */
		} /* end if */

		/* check for process separation */
		definition->separate = HAS_ATTR_VALUE (pdef, "separate", "yes");

		/* check for chroot value */
		definition->chroot   = ATTR_VALUE (pdef, "chroot");

		/* check for chroot value */
		definition->work_dir = ATTR_VALUE (pdef, "work-dir");

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
	vortex_listener_set_on_connection_accepted (vortex_ctx, 
						    __turbulence_ppath_handle_connection_on_connect, 
						    ctx); 
	vortex_connection_set_connection_actions   (vortex_ctx,
						    CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES,
						    __turbulence_ppath_handle_connection_on_greetings,
						    ctx);
	
	msg ("profile path definition ok (all rules address based status: %d)..", ctx->all_rules_address_based);

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
			turbulence_expr_free (def->dst);

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

	return;
}

/** 
 * @internal Change to the effective user id and group id configured
 * in the profile path configuration (if any).
 */
void turbulence_ppath_change_user_id (TurbulenceCtx      * ctx, 
				      TurbulencePPathDef * ppath_def)
{

	/* check to change current group */
	if (ppath_def->group_id != -1 && ppath_def->group_id > 0) {
		if (setgid (ppath_def->group_id) != 0) {
			error ("Failed to set executing group id: %d, error (%d:%s)", 
			       ppath_def->group_id, errno, vortex_errno_get_last_error ());
		} /* end if */
	} /* end if */

	/* check to change current user */
	if (ppath_def->user_id != -1 && ppath_def->user_id > 0) {
		if (setuid (ppath_def->user_id) != 0) {
			error ("Failed to set executing user id: %d, error (%d:%s)", 
			       ppath_def->user_id, errno, vortex_errno_get_last_error ());
		} /* end if */
	} /* end if */

	/* update process executing ids */
	ppath_def->user_id = getuid ();
	ppath_def->group_id = getgid ();
	msg ("running process as: %d:%d", ppath_def->user_id, ppath_def->group_id);

	return;
}

/** 
 * @internal Allows to get the current selected profile path on the
 * provided connection.
 *
 * @param conn The connection where the Profile path configured is requested.
 *
 * @return The profile path name or NULL if it has no profile path
 * defined.
 */
TurbulencePPathDef * turbulence_ppath_selected (VortexConnection * conn)
{
	TurbulencePPathState * state;

	/* check connection reference */
	v_return_val_if_fail (conn, axl_false);

	/* get state */
	state = vortex_connection_get_data (conn, TURBULENCE_PPATH_STATE);
	if (state == NULL || state->path_selected == NULL)
		return NULL;
	
	/* return path name */
	return state->path_selected;
}

/** 
 * @brief Allows to get profile path name from the provided profile
 * path.
 * @param ppath_def The profile path definition where the name is retrieved.
 *
 * @return A reference to the profile path name or NULL it if fails.
 */
const char         * turbulence_ppath_get_name (TurbulencePPathDef * ppath_def)
{
	if (ppath_def == NULL)
		return NULL;
	return ppath_def->path_name;
}

/** 
 * @brief Allows to get the profile path working directory.
 *
 * @param ppath_def The profile path definition where the work
 * directory is retrieved.
 *
 * @return A reference to the work directory defined on the profile
 * path or NULL it is not defined.
 */
const char         * turbulence_ppath_get_work_dir    (TurbulenceCtx      * ctx,
						       TurbulencePPathDef * ppath_def)
{
	DIR           * directory;

	if (ppath_def == NULL)
		return NULL;
	if (ppath_def->work_dir && strlen (ppath_def->work_dir) > 0) {
		/* check that the directory path is indeed a directory and we can open it */
		directory = opendir (ppath_def->work_dir);
		if (directory == NULL) {
			wrn ("Defined a work directory for profile path not accesible: %s", ppath_def->work_dir);
			return NULL;
		}
		closedir (directory);

		return ppath_def->work_dir;
	}
	return NULL;
} 

/** 
 * @brief Allows to get the serverName requested for the profile path
 * selected. This value represents the serverName sent on channel
 * start request and was used, along with other datas, to select
 * current profile path. This value may be NULL since serverName is
 * optional.
 *
 * @param ppath_def The profile path selected where the serverName
 * requested will be returned.
 *
 * @return A reference to the serverName value or NULL if it fails.
 *
 * <b>NOTE: this is NOT the serverName. In many cases they are the
 * same value, but it must be considered as a requested value, until
 * the first channel with serverName is accepted. Then a call to
 * vortex_connection_get_server_name can be done. </b>
 */
const char         * turbulence_ppath_get_server_name (VortexConnection * conn)
{
	TurbulencePPathState * state;
	TurbulenceCtx        * ctx;
	if (conn == NULL)
		return NULL;

	/* get state */
	state = vortex_connection_get_data (conn, TURBULENCE_PPATH_STATE);
	if (state == NULL)
		return NULL;

	/* get context */
	ctx   = state->ctx;
	msg ("returing serverName on state %p, %p", state, state->requested_serverName);
	return state->requested_serverName;
}

#if defined(DEFINE_CHROOT_PROTO)
int  chroot (const char * path);
#endif

/** 
 * @internal Allows to change current process root dir.
 */
void turbulence_ppath_change_root    (TurbulenceCtx      * ctx, 
				      TurbulencePPathDef * ppath_def)
{
	/* check for permission */
	if (getuid () != 0) 
		return;
	/* check if chroot is defined */
	if (ppath_def->chroot == NULL)
		return;
	if (chroot (ppath_def->chroot) != 0) 
		error ("Failed to change root dir to %s, error found: %d:%s", 
		       ppath_def->chroot,
		       errno, vortex_errno_get_last_error ());
	msg ("change root dir to: %s", ppath_def->chroot);
	return;
}

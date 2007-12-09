/*
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
#include <mod-tunnel.h>

axlDoc          * mod_tunnel_resolver = NULL;
axlDoc          * tunnel_conf         = NULL;
TurbulenceCtx   * ctx              = NULL;

bool find_and_replace (const char * conf, axlNode * node)
{
	axlNode    * db;
	const char * value = ATTR_VALUE (node, conf);

	/* lookup for the value at the provided conf */
	db = axl_doc_get (mod_tunnel_resolver, "/tunnel-resolver/match");
	while (db != NULL) {
		/* check node found */
		if (HAS_ATTR_VALUE (db, conf, value)) {
			/* remove current node */
			axl_node_remove_attribute (node, conf);

			/* configure new values, check host */
			value = ATTR_VALUE (node, "host");
			if (axl_memcmp (value, "ip4:", 4)) {
				/* ip4 case */
				axl_node_set_attribute (node, "ip4", value + 4);

			} else if (axl_memcmp (value, "fqdn:", 5)) {
				/* fqdn case */
				axl_node_set_attribute (node, "fqdn", value + 5);

			} else {
				/* ip4 case, default */
				axl_node_set_attribute (node, "ip4", value + 4);
			}

			/* configure new values, check port */
			value = ATTR_VALUE (node, "port");
			axl_node_set_attribute (node, "port", value);

			return true;
		} /* node found ! */

		/* get next node */
		db = axl_node_get_next (db);
	} /* end if */

	return false;
}

/** 
 * @brief Tunnel settings resolver implementation.
 */
VortexTunnelSettings * tunnel_resolver (const char * profile_content,
					int          profile_content_size,
					axlDoc     * doc,
					axlPointer   user_data)
{
	
	axlNode              * node = NULL;
	char                 * content;
	int                    size;
	VortexTunnelSettings * settings;

	/* get root, that is, the next hop */
	node = axl_doc_get_root (doc);

	/* check profile and endpoint */
	if (! HAS_ATTR (node, "endpoing") && ! HAS_ATTR (node, "profile"))
		return NULL;

	/* check and translate */
	if (HAS_ATTR (node, "endpoint")) {
		/* try to translate endpoing */
		find_and_replace ("endpoint", node);
	} /* end if */

	/* check and translate */
	if (HAS_ATTR (node, "profile")) {
		/* try to translate endpoing */
		find_and_replace ("profile", node);
	} /* end if */

	/* dump the content translated, and free document */
	axl_doc_dump (doc, &content, &size);

	/* create settings, free content and return value */
	settings = vortex_tunnel_settings_new_from_xml (content, size);
	axl_free (content);
	return settings;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other task. The function must
 * return true to signal that the module was initialized
 * ok. Otherwise, false must be returned.
 */
static bool tunnel_init (TurbulenceCtx * _ctx)
{
	axlNode  * node;
	axlError * error;
	char     * config;

	/* prepare mod-sasl module */
	TBC_MOD_PREPARE (_ctx);
	
	msg ("turbulence TUNNEL init");

	/* check if vortex library supports the tunnel profile */
	if (! vortex_tunnel_is_enabled ()) {
		error ("found vortex library without TUNNEL support (compile one with it included!)");
		return false;
	} /* end if */

	/* configure lookup domain for mod tunnel settings */
	vortex_support_add_domain_search_path_ref (axl_strdup ("tunnel"), 
						   vortex_support_build_filename (SYSCONFDIR, "turbulence", "tunnel", NULL));

	/* load module settings */
	config       = vortex_support_domain_find_data_file ("tunnel", "tunnel.conf");
	tunnel_conf  = axl_doc_parse_from_file (config, &error);
	axl_free (config);
	if (tunnel_conf == NULL) {
		error ("failed to init the TUNNEL profile, unable to find configuration file, error: %s",
		       axl_error_get (error));
		axl_error_free (error);
		return false;
	} /* end if */
	
	/* init translation database */
	node                = axl_doc_get (tunnel_conf, "/mod-tunnel/tunnel-resolver");
	if (HAS_ATTR_VALUE (node, "type", "xml") &&
	    HAS_ATTR (node, "location")) {
		/* find the file to load */
		config              = vortex_support_domain_find_data_file ("tunnel", ATTR_VALUE (node, "location"));
		mod_tunnel_resolver = axl_doc_parse_from_file (config, NULL);
		if (mod_tunnel_resolver == NULL) {
			error ("failed to open resolver file, TUNNEL translation settings will not be applied, error: %s",
			       axl_error_get (error));
			axl_error_free (error);
		} /* end if */
		msg ("database resolver for TUNNEL loaded: %s", config);
		axl_free (config);
	} /* end if */

	/* activates the tunnel profile to accept connections
	 * (tunneling them or forwarding them to the next hop) */
	if (! vortex_tunnel_accept_negotiation (NULL, NULL)) {
		error ("failed to init the TUNNEL profile");
		return false;
	} /* end if */

	/* configure tunnel settings translation */
	vortex_tunnel_set_resolver (tunnel_resolver, NULL);
		
	return true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void tunnel_close (TurbulenceCtx * ctx)
{
	msg ("turbulence TUNNEL close");
	axl_doc_free (tunnel_conf);
	axl_doc_free (mod_tunnel_resolver);
}

/**
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-tunnel",
	"BEEP proxy, TUNNEL profile",
	tunnel_init,
	tunnel_close,
	/* no reconf function for now */
	NULL
};


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

/* include local DTD */
#include <turbulence-config.dtd.h>

/** 
 * \defgroup turbulence_config Turbulence Config: files to access to run-time Turbulence Config
 */

/**
 * \addtogroup turbulence_config
 * @{
 */

/** 
 * @internal Loads the turbulence main file, which has all definitions to make
 * turbulence to start.
 * 
 * @param config The configuration file to load by the provided
 * turbulence context.
 * 
 * @return axl_true if the configuration file looks ok and it is
 * syncatically correct.
 */
axl_bool  turbulence_config_load (TurbulenceCtx * ctx, const char * config)
{
	axlError   * error;
	axlDtd     * dtd_file;


	/* check null value */
	if (config == NULL) {
		error ("config file not defined, terminating turbulence");
		return axl_false;
	} /* end if */

	/* load the file */
	ctx->config = axl_doc_parse_from_file (config, &error);
	if (ctx->config == NULL) {
		error ("unable to load file (%s), it seems a xml error: %s", 
		       config, axl_error_get (error));

		/* free resources */
		axl_error_free (error);

		/* call to finish turbulence */
		return axl_false;

	} /* end if */

	/* drop a message */
	msg ("file %s loaded, ok", config);

	/* found dtd file */
	dtd_file = axl_dtd_parse (TURBULENCE_CONFIG_DTD, -1, &error);
	if (dtd_file == NULL) {
		axl_doc_free (ctx->config);
		error ("unable to load DTD to validate turbulence configuration, error: %s", axl_error_get (error));
		axl_error_free (error);
		return axl_false;
	} /* end if */

	if (! axl_dtd_validate (ctx->config, dtd_file, &error)) {
		abort_error ("unable to validate server configuration, something is wrong: %s", 
			     axl_error_get (error));

		/* free and set a null reference */
		axl_doc_free (ctx->config);
		ctx->config = NULL;

		axl_error_free (error);
		return axl_false;
	} /* end if */

	msg ("server configuration is valid..");
	
	/* free resources */
	axl_dtd_free (dtd_file);

	return axl_true;
}

/** 
 * @brief Allows to get the configuration loaded at the startup. The
 * function will always return a configuration object. 
 * 
 * @return A reference to the axlDoc having all the configuration
 * loaded.
 */
axlDoc * turbulence_config_get (TurbulenceCtx * ctx)
{
	/* null for the caller if a null is received */
	v_return_val_if_fail (ctx, NULL);

	/* return current reference */
	return ctx->config;
}

/** 
 * @brief Allows to configure the provided name and value on the
 * provided path inside the turbulence config.
 *
 * @param ctx The turbulence context where the configuration will be
 * modified.
 *
 * @param path The xml path to the configuration to be modified.
 *
 * @param attr_name The attribute name to be modified on the selected
 * path node.
 *
 * @param attr_value The attribute value to be modified on the
 * selected path node. 
 *
 * @return axl_true if the value was configured, otherwise axl_false
 * is returned (telling the value wasn't configured mostly because the
 * path is wrong or the node does not exists or any of the values
 * passed to the function is NULL).
 */
axl_bool            turbulence_config_set      (TurbulenceCtx * ctx,
						const char    * path,
						const char    * attr_name,
						const char    * attr_value)
{
	axlNode * node;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name && attr_value, axl_false);

	msg ("Setting value %s=%s at path %s (%s)", attr_name, attr_value, path, attr_name);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) {
		wrn ("  Path %s was not found in config (%p)", path, ctx->config);
		return axl_false;
	} /* end if */

	/* set attribute */
	axl_node_remove_attribute (node, attr_name);
	axl_node_set_attribute (node, attr_name, attr_value);
	
	return axl_true;
}

/** 
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The turbulence context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 */
axl_bool        turbulence_config_is_attr_positive (TurbulenceCtx * ctx,
						    axlNode       * node,
						    const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "yes"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "1"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "true"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}

/**
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The turbulence context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 */
axl_bool        turbulence_config_is_attr_negative (TurbulenceCtx * ctx,
						    axlNode       * node,
						    const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "no"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "0"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "false"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}


/** 
 * @brief Allows to get the value found on provided config path at the
 * selected attribute.
 *
 * @param ctx The turbulence context where to get the configuration value.
 *
 * @param path The path to the node where the config is found.
 *
 * @param attr_name The attribute name to be returned as a number.
 *
 * @return The function returns the value configured or -1 in the case
 * the configuration is wrong. The function returns -2 in the case
 * path, ctx or attr_name are NULL. The function returns -3 in the
 * case the path is not found so the user can take default action.
 */
int             turbulence_config_get_number (TurbulenceCtx * ctx, 
					      const char    * path,
					      const char    * attr_name)
{
	axlNode * node;
	int       value;
	char    * error = NULL;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name, -2);

	msg ("Getting value at path %s (%s)", path, attr_name);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) {
		wrn ("  Path %s was not found in config (%p)", path, ctx->config);
		return -3;
	}

	msg ("  Translating value to a number %s=%s", attr_name, ATTR_VALUE (node, attr_name));

	/* now get the value */
	value = vortex_support_strtod (ATTR_VALUE (node, attr_name), &error);
	if (error && strlen (error) > 0) 
		return -1;
	return value;
}

/** 
 * @internal Cleanups the turbulence config module. This is called by
 * Turbulence itself on exit.
 */
void turbulence_config_cleanup (TurbulenceCtx * ctx)
{
	/* do not operate */
	if (ctx == NULL)
		return;

	/* free previous state */
	if (ctx->config)
		axl_doc_free (ctx->config);
	ctx->config = NULL;

	return;
} 

/* @} */

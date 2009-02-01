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
int  turbulence_config_load (TurbulenceCtx * ctx, const char * config)
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
		error ("unable to validate server configuration, something is wrong: %s", 
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

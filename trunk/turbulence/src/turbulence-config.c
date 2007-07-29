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
#include <turbulence.h>

axlDoc * __turbulence_config = NULL;

/** 
 * Loads the turbulence main file, which has all definitions to make
 * turbulence to start.
 * 
 * @param config 
 * 
 * @return true if the configuration file looks ok and it is
 * syncatically correct.
 */
bool turbulence_config_load (char * config)
{
	axlError * error;
	axlDtd   * dtd_file;
	char     * dtd;

	/* check null value */
	if (config == NULL) {
		error ("config file not defined, terminating turbulence");
		turbulence_exit (-1);
		return false;
	} /* end if */

	/* load the file */
	__turbulence_config = axl_doc_parse_from_file (config, &error);
	if (__turbulence_config == NULL) {
		error ("unable to load file (%s), it seems a xml error: %s", 
		       config, axl_error_get (error));

		/* free resources */
		axl_free (config);
		axl_error_free (error);

		/* call to finish turbulence */
		turbulence_exit (-1);
		return false;

	} /* end if */
	
	/* drop a message */
	msg ("file %s loaded, ok", config);

	/* free resources */
	axl_free (config);

	/* now validates the turbulence file */
	dtd = vortex_support_domain_find_data_file ("turbulence-data", "config.dtd");
	if (dtd == NULL) {
		/* free document */
		axl_doc_free (__turbulence_config);
		error ("unable to find turbulence config DTD definition (config.dtd), check your turbulence installation.");

		turbulence_exit (-1);
		return false;
	} /* end if */
 
	/* found dtd file */
	msg ("found dtd file at: %s", dtd);
	dtd_file = axl_dtd_parse_from_file (dtd, &error);
	if (dtd_file == NULL) {
		axl_doc_free (__turbulence_config);
		error ("unable to load DTD file %s, error: %s", dtd, axl_error_get (error));
		axl_error_free (error);
		axl_free (dtd);

		turbulence_exit (-1);
		return false;
	} /* end if */

	if (! axl_dtd_validate (__turbulence_config, dtd_file, &error)) {
		error ("unable to validate server configuration (%s), something is wrong: %s", 
		       dtd, axl_error_get (error));

		/* free and set a null reference */
		axl_doc_free (__turbulence_config);
		__turbulence_config = NULL;

		axl_error_free (error);
		axl_free (dtd);

		turbulence_exit (-1);
		return false;
	} /* end if */

	msg ("server configuration is valid..");
	
	/* free resources */
	axl_dtd_free (dtd_file);
	axl_free (dtd);

	return true;
}

/** 
 * @brief Allows to get the configuration loaded at the startup. The
 * function will always return a configuration object. If the
 * configuration was not properly setup, \ref turbulence_config_load
 * will stop execution, reporting to the user.
 * 
 * @return A reference to the axlDoc having all the configuration
 * created. 
 */
axlDoc * turbulence_config_get ()
{
	/* return current reference */
	return __turbulence_config;
}

/** 
 * @brief Cleanups the turbulence config module.
 */
void turbulence_config_cleanup ()
{
	/* free previous state */
	if (__turbulence_config)
		axl_doc_free (__turbulence_config);
	__turbulence_config = NULL;

	return;
} 



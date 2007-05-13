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
 *         info@aspl.es - http://fact.aspl.es
 */
#include <common-sasl.h>

long int      sasl_xml_db_time = 0;

/** 
 * @internal Loads the sasl configuration, returning the proper
 * variables filled.
 */
bool common_sasl_load_config (axlDoc      ** sasl_xml_conf, 
			      char        ** sasl_xml_db_path, 
			      axlDoc      ** sasl_xml_db,
			      VortexMutex  * mutex)
{
	char     * config;
	axlNode  * node;
	axlError * error;

	/* configure lookup domain for mod tunnel settings */
	vortex_support_add_domain_search_path_ref (axl_strdup ("sasl"), 
						   vortex_support_build_filename (SYSCONFDIR, "turbulence", "sasl", NULL));

	/* find and load the file */
	config          = vortex_support_domain_find_data_file ("sasl", "sasl.conf");
	*sasl_xml_conf  = axl_doc_parse_from_file (config, &error);
	axl_free (config);
	
	/* check result */
	if (*sasl_xml_conf == NULL) {
		error ("failed to init the SASL profile, unable to find configuration file, error: %s",
		       axl_error_get (error));
		axl_error_free (error);
		return false;
	} /* end if */

	/* now load the users db */
	node                = axl_doc_get (*sasl_xml_conf, "/mod-sasl/auth-db");
	if (HAS_ATTR_VALUE (node, "type", "xml") &&
	    HAS_ATTR (node, "location")) {

		/* find file */
		*sasl_xml_db_path  = vortex_support_domain_find_data_file ("sasl", ATTR_VALUE (node, "location"));

		/* load db */
		if (! common_sasl_load_users_db (sasl_xml_db, *sasl_xml_db_path, mutex))
			return false;

	} /* end if */

	return true;
}

/** 
 * @internal Loads the xml users database into memory.
 * 
 * @return true if the db was properly loaded.
 */
bool common_sasl_load_users_db (axlDoc ** sasl_xml_db, char * sasl_xml_db_path, VortexMutex * mutex)
{
	axlError * error;

	/* lock the mutex */
	vortex_mutex_lock (mutex);

	/* check file modification */
	if (turbulence_last_modification (sasl_xml_db_path) == sasl_xml_db_time) {
		
		/* lock the mutex */
		vortex_mutex_unlock (mutex);

		return true;
	} /* end if */

	msg ("loading sasl auth xml-db..");

	/* load the db */
	if (sasl_xml_db != NULL)
		axl_doc_free (*sasl_xml_db);
	
	/* find the file to load */
	*sasl_xml_db       = axl_doc_parse_from_file (sasl_xml_db_path, &error);
	
	/* check db opened */
	if (sasl_xml_db == NULL) {
		/* unlock the mutex */
		vortex_mutex_unlock (mutex);

		error ("failed to init the SASL profile, unable to auth db, error: %s",
		       axl_error_get (error));
		axl_error_free (error);
		return false;
	} /* end if */
	
	/* get current db time */
	sasl_xml_db_time = turbulence_last_modification (sasl_xml_db_path);

	/* unlock the mutex */
	vortex_mutex_unlock (mutex);
	
	return true;
}



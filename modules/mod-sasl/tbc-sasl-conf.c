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

#define HELP_HEADER "tbc-sasl-conf: a tool to administrate mod-sasl and users.\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

#include <turbulence.h>
#include <common-sasl.h>

axlDoc      * sasl_xml_conf = NULL;
axlDoc      * auth_db       = NULL;
char        * auth_db_path  = NULL;

void tbc_sasl_add_user ()
{
	const char * new_user_id = exarg_get_string ("add-user");
	const char * password    = NULL;
	const char * user_id;
	axlNode    * node;
	axlNode    * newNode;

	/* check if the user already exists */
	node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
	while (node != NULL) {
		
		/* get the user */
		user_id = ATTR_VALUE (node, "user_id");
		
		/* check the format for the user stored */
		if (axl_memcmp (user_id, "text:", 5)) {
			/* check the user id */
			if (axl_cmp (new_user_id, user_id + 5)) {
				msg ("user %s already exist..", new_user_id);
				return;
			} /* end if */
		} /* end if */

		/* get next node */
		node     = axl_node_get_next (node);

	} /* end while */

	/* user doesn't exist, ask for the password */
	if (exarg_is_defined ("password")) 
		password = exarg_get_string ("password");
	else {
		error ("password for user %s not provided", new_user_id);
	}

	/* check if the user already exists */
	node     = axl_doc_get_root (auth_db);
	newNode  = axl_node_create ("auth");
	
	/* set user */
	axl_node_set_attribute_ref (newNode, 
				    axl_strdup ("user_id"),
				    axl_strdup_printf ("text:%s", new_user_id));

	/* set password */
	axl_node_set_attribute_ref (newNode, 
				    axl_strdup ("password"),
				    axl_strdup_printf ("text:%s", password));

	/* account enabled */
	axl_node_set_attribute (newNode, "disabled", "no");
				    
	/* set the node */
	axl_node_set_child (node, newNode);

	/* dump the db */
	if (! axl_doc_dump_pretty_to_file (auth_db, auth_db_path, 3))
		error ("failed to dump SASL auth db..");
	else
		msg ("user %s added!", new_user_id);
	return;
}

void tbc_sasl_disable_user ()
{
	
}

int main (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("add-user", "a", EXARG_STRING, 
			   "Adds a new users to the sasl database");

	/* install exarg options */
	exarg_install_arg ("disable-user", "d", EXARG_STRING, 
			   "Makes the user to be available on the system but not usable to perform real auth operation.");

	/* call to parse arguments */
	exarg_parse (argc, argv);
	
	/* enable console log */
	turbulence_set_console_debug (true);

	/* check empty arguments */
	if (argc == 1) {
		msg ("Try to use: %s --help", argv[0]);
	}

	/* load sasl module configuration */
	if (! common_sasl_load_config (&sasl_xml_conf, &auth_db_path, &auth_db, NULL))
		goto finish;

	msg ("using db: %s", auth_db_path);
	
	/* check if the user want to add a new user */
	if (exarg_is_defined ("add-user")) {
		tbc_sasl_add_user ();
	} else if (exarg_is_defined ("disable-user")) {
		tbc_sasl_disable_user ();
	}


 finish:

	/* terminate the library */
	exarg_end ();

	/* free document */
	axl_doc_free (sasl_xml_conf);

	return 0;
}

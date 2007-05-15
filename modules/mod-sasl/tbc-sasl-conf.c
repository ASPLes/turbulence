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
	const char * password2   = NULL;
	const char * user_id;
	bool         dealloc     = false;
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
	msg ("adding user: %s..", new_user_id);
	if (exarg_is_defined ("password")) 
		password = exarg_get_string ("password");
	else {
		password  = turbulence_io_get ("Password: ", DISABLE_STDIN_ECHO);
		fprintf (stdout, "\n");
		password2 = turbulence_io_get ("Type again: ", DISABLE_STDIN_ECHO);
		fprintf (stdout, "\n");
		if (! axl_cmp (password, password2)) {
			axl_free ((char*) password);
			axl_free ((char*) password2);
			error ("Password mismatch..");
			return;
		} else {
			axl_free ((char*) password2);
		}
		dealloc = true;
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

	/* free the password */
	if (dealloc)
		axl_free ((char*) password);

	/* dump the db */
	if (! axl_doc_dump_pretty_to_file (auth_db, auth_db_path, 3))
		error ("failed to dump SASL auth db..");
	else
		msg ("user %s added!", new_user_id);
	return;
}

/** 
 * @internal Lookup for the user and disables it.
 */
void tbc_sasl_disable_user ()
{
	const char * user_id_to_disable = exarg_get_string ("disable-user");
	const char * user_id;
	axlNode    * node;

	/* check if the user already exists */
	node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
	while (node != NULL) {
		
		/* get the user */
		user_id = ATTR_VALUE (node, "user_id");
		
		/* check the format for the user stored */
		if (axl_memcmp (user_id, "text:", 5)) {
			/* check the user id */
			if (axl_cmp (user_id_to_disable, user_id + 5)) {
				/* user found, disable it */
				axl_node_remove_attribute (node, "disabled");

				/* install the new attribute */
				axl_node_set_attribute (node, "disabled", "yes");


				/* dump the db */
				if (! axl_doc_dump_pretty_to_file (auth_db, auth_db_path, 3))
					error ("failed to dump SASL auth db..");
				else
					msg ("user %s disabled!", user_id_to_disable);
				return;
			} /* end if */
		} /* end if */

		/* get next node */
		node     = axl_node_get_next (node);

	} /* end while */

	/* nothing to do */
	return;
	
} /* end tbc_sasl_disable_user */

/** 
 * @internal List all users created.
 */
void tbc_sasl_list_users ()
{
	axlNode    * node;
	const char * user_id;
	bool         first_user = true;

	/* check if the user already exists */
	node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
	while (node != NULL) {

		if (first_user) {
			msg ("Number of users created (%d):", axl_node_get_child_num (axl_node_get_parent (node)));
			first_user = false;
		} /* end if */
		
		/* check the format for the user stored */
		if (axl_memcmp (ATTR_VALUE (node, "user_id"), "text:", 5)) {

			/* get the actual user id */
			user_id = ATTR_VALUE (node, "user_id") + 5;

			msg ("  %s (disabled=%s)", 
			     user_id, ATTR_VALUE (node, "disabled"));

		} /* end if */

		/* get next node */
		node     = axl_node_get_next (node);

	} /* end while */

	/* nothing to do */
	return;
	
} /* end tbc_sasl_disable_user */

/** 
 * @internal Lookup for the user and removes it from the database.
 */
void tbc_sasl_remove_user ()
{
	const char * user_id_to_remove = exarg_get_string ("remove-user");
	const char * user_id;
	axlNode    * node;

	/* check if the user already exists */
	node     = axl_doc_get (auth_db, "/sasl-auth-db/auth");
	while (node != NULL) {
		
		/* get the user */
		user_id = ATTR_VALUE (node, "user_id");
		
		/* check the format for the user stored */
		if (axl_memcmp (user_id, "text:", 5)) {
			/* check the user id */
			if (axl_cmp (user_id_to_remove, user_id + 5)) {

				/* remove the node */
				axl_node_remove (node, true);

				/* dump the db */
				if (! axl_doc_dump_pretty_to_file (auth_db, auth_db_path, 3))
					error ("failed to dump SASL auth db..");
				else
					msg ("user %s removed!", user_id_to_remove);
				return;
			} /* end if */
		} /* end if */

		/* get next node */
		node     = axl_node_get_next (node);

	} /* end while */

	/* nothing to do */
	return;

} /* end tbc_sasl_remove_user */

int main (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("add-user", "a", EXARG_STRING, 
			   "Adds a new users to the sasl database.");
	exarg_install_arg ("password", "p", EXARG_STRING,
			   "Configures the password for the operation. Optionally used by --add-user");

	/* install exarg options */
	exarg_install_arg ("disable-user", "d", EXARG_STRING, 
			   "Makes the user to be available on the system but not usable to perform real auth operation.");

	exarg_install_arg ("remove-user", "r", EXARG_STRING,
			   "Removes the user provided from the sasl database.");

	exarg_install_arg ("list-users", "l", EXARG_NONE,
			   "Allows to list current users created.");


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

		/* add a user */
		tbc_sasl_add_user ();
	} else if (exarg_is_defined ("disable-user")) {

		/* disable a user */
		tbc_sasl_disable_user ();
	} else if (exarg_is_defined ("remove-user")) {

		/* remove a user */
		tbc_sasl_remove_user ();
	} else if (exarg_is_defined ("list-users")) {

		/* list all users */
		tbc_sasl_list_users ();
	}


 finish:

	/* terminate the library */
	exarg_end ();

	/* free document */
	axl_doc_free (sasl_xml_conf);

	return 0;
}

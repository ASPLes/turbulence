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

#define HELP_HEADER "tbc-sasl-conf: a tool to administrate mod-sasl and users.\n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

#include <turbulence.h>
#include <common-sasl.h>


SaslAuthBackend * sasl_backend = NULL;

void tbc_sasl_add_user ()
{
	const char  * serverName   = NULL;
	const char  * new_user_id  = exarg_get_string ("add-user");
	const char  * password     = NULL;
	const char  * password2    = NULL;
	bool          dealloc     = false;

	/* check if the user already exists */
	if (common_sasl_user_exists (sasl_backend,
				     new_user_id, 
				     serverName,
				     NULL,
				     NULL)) {
		msg ("user %s already exists..", new_user_id);
		return;
	} /* end if */
				     
				     
	/* user doesn't exist, ask for the password */
	msg ("adding user: %s..", new_user_id);
	if (exarg_is_defined ("password")) 
		password = exarg_get_string ("password");
	else {
		/* user didn't provide a password, get from the
		 * command line */
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


	/* add the user */
	if (! common_sasl_user_add (sasl_backend, new_user_id, password, serverName, NULL))
		error ("failed to dump SASL auth db..");
	else
		msg ("user %s added!", new_user_id);

	if (dealloc)
		axl_free ((char*) password);

	return;
}

/** 
 * @internal Lookup for the user and disables it.
 */
void tbc_sasl_disable_user ()
{
	const char * user_id_to_disable = exarg_get_string ("disable-user");
	const char * serverName         = NULL;

	/* call to disable user */
	if (! common_sasl_user_disable (sasl_backend, user_id_to_disable, serverName, true, NULL))
		error ("failed to dump SASL auth db..");
	else
		msg ("user %s disabled!", user_id_to_disable);
	/* nothing to do */
	return;
	
} /* end tbc_sasl_disable_user */

/** 
 * @internal List all users created.
 */
void tbc_sasl_list_users ()
{
	const char * serverName = NULL;
	axlList    * list;
	int          iterator   = 0;
	SaslUser   * user;

	/* get the list of users installed */
	list = common_sasl_get_users (sasl_backend, serverName, NULL);
	msg ("Number of users created (%d):", list != NULL ? axl_list_length (list) : 0);
	while (iterator < axl_list_length (list)) {

		/* get a reference */
		user = axl_list_get_nth (list, iterator);

		msg ("  %s (disabled=%s)", 
		     user->auth_id, user->disabled ? "yes" : "no");

		/* update iterator value */
		iterator++;
		
	} /* end if */

	/* free the list */
	axl_list_free (list);
	return;

} /* end tbc_sasl_disable_user */

/** 
 * @internal Lookup for the user and removes it from the database.
 */
void tbc_sasl_remove_user ()
{
	const char * serverName        = NULL;
	const char * user_id_to_remove = exarg_get_string ("remove-user");

	/* call to remove the user */
	if (! common_sasl_user_remove (sasl_backend, user_id_to_remove, serverName, NULL)) 
		error ("failed to dump SASL auth db..");
	else
		msg ("user %s removed!", user_id_to_remove);
	return;

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
	exarg_install_arg ("disable-user", "e", EXARG_STRING, 
			   "Makes the user to be available on the system but not usable to perform real auth operation.");

	exarg_install_arg ("remove-user", "r", EXARG_STRING,
			   "Removes the user provided from the sasl database.");

	exarg_install_arg ("list-users", "l", EXARG_NONE,
			   "Allows to list current users created.");

	/* do not accept free arguments */
	exarg_accept_free_args (0);

	/* install turbulence tool options */
	turbulence_console_install_options ();
	turbulence_db_list_init ();

	/* call to parse arguments */
	exarg_parse (argc, argv);
	
	/* process turbulence tool options */
	turbulence_console_process_options ();

	/* check empty arguments */
	if (argc == 1) {
		msg ("Try to use: %s --help", argv[0]);
		goto finish;
	}

	/* load sasl module configuration */
	if (! common_sasl_load_config (&sasl_backend, NULL, NULL))
		goto finish;
	
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

	/* free sasl backend */
	common_sasl_free (sasl_backend); 

	/* stop pieces of turbulence started */
	turbulence_db_list_cleanup ();
	vortex_support_cleanup ();

	return 0;
}

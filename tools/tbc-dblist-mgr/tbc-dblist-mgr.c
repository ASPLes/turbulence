/*  Turbulence BEEP application server
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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

/* include axl support */
#include <axl.h>

/* include exarg support */
#include <exarg.h>

/* include turbulence */
#include <turbulence.h>

#define HELP_HEADER "tbc-dblist-mgr: a tool to manage turbulence db lists\n\
Copyright (C) 2025 Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
[GENERAL HELP]\n\n\
To add a new item into a particular db list use:\n\n\
    tbc-dblist-mgr --add 'some value' db-list.xml\n\n\
To remove an item from a particular list use:\n\n\
    tbc-dblist-mgr --remove 'some value' db-list.xml\n\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/* " */

int main (int argc, char ** argv)
{
	TurbulenceDbList * list;
	axlList          * content;
	axlListCursor    * cursor;
	axlError         * err;
	ExArgument       * arg;
	TurbulenceCtx    * ctx;
	VortexCtx        * vortex_ctx;

	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Provides tool version");

	exarg_install_arg ("add", "a", EXARG_STRING, 
			   "Allows to add the provide value into the db-list selected");

	exarg_install_arg ("remove", "r", EXARG_STRING, 
			   "Allows to remove the provide value from the db-list selected");

	exarg_install_arg ("list", "l", EXARG_NONE, 
			   "List the content inside the provided db list.");

	exarg_install_arg ("touch", "t", EXARG_NONE, 
			   "Creates an empty db-list.");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified.");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* init turbulence context */
	ctx        = turbulence_ctx_new ();
	vortex_ctx = vortex_ctx_new ();
	
	/* bind vortex ctx */
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);

	/* configure context debug according to values received */
	turbulence_log_enable  (ctx, true);
	turbulence_log2_enable (ctx, exarg_is_defined ("debug2"));
	turbulence_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* check console color debug */
	turbulence_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* check version argument */
	if (exarg_is_defined ("version")) {
		/* free turbulence context */
		turbulence_ctx_free (ctx);

		msg ("%s version: %s", argv[0], VERSION);
		return 0;
	}

	if (exarg_get_params_num () == 0) {
		/* free turbulence context */
		turbulence_ctx_free (ctx);

		error ("you didn't provide the db-list file to oper.");
		return -1;
	}

	/* get the argument */
	arg = exarg_get_params ();

	/* update the search path to make the default installation to
	 * work at the default location. */
	vortex_support_init (vortex_ctx);
	vortex_support_add_domain_search_path (vortex_ctx, "turbulence-data", turbulence_datadir (ctx));
	
	/* init the turbulence db list */
	if (! turbulence_db_list_init (ctx)) {
		error ("failed to init the turbulence db list module, unable to perform operation");
		return -1;
	}

	/* open the list */
	list = turbulence_db_list_open (ctx, &err, exarg_param_get (arg), NULL);
	if (list == NULL) {
		error ("failed to open db-list: %s, error was: %s", 
		       exarg_param_get (arg), axl_error_get (err));
		axl_error_free (err);
		return -1;
	}

	if (exarg_is_defined ("add")) {
		/* do add operation */
		if (! turbulence_db_list_add (list, exarg_get_string ("add"))) {
			error ("failed to add provided value: %s", exarg_get_string ("add"));
			return -1;
		}

		msg ("done");
	} else if (exarg_is_defined ("remove")) {
		/* do remove operation */
		if (! turbulence_db_list_remove (list, exarg_get_string ("remove"))) {
			error ("failed to remove provided value: %s", exarg_get_string ("remove"));
			return -1;
		}
		msg ("done");

	} else if (exarg_is_defined ("touch")) {
		
		/* ...and close */
		turbulence_db_list_close (list);

	} else if (exarg_is_defined ("list")) {
		/* get current content */
		content = turbulence_db_list_get (list);
		cursor  = axl_list_cursor_new (content);

		/* for each item found in the cursor */
		msg ("items found:");
		while (axl_list_cursor_has_item (cursor)) {
			
			/* get the content */
			printf ("  %s\n", (char*) axl_list_cursor_get (cursor));

			/* jump to the next */
			axl_list_cursor_next (cursor);

		} /* end if */
		
		/* free resources */
		axl_list_free (content);
		axl_list_cursor_free (cursor);
	} 

	/* terminate turbulence db list */
	turbulence_db_list_cleanup (ctx);

	/* free context */
	turbulence_ctx_free (ctx);
	vortex_ctx_free (vortex_ctx);

	return 0;
}


/*  tbc-mod-gen: A tool to produce modules for the Turbulence BEEP server
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

/* include axl support */
#include <axl.h>

/* include exarg support */
#include <exarg.h>

/* include turbulence */
#include <turbulence.h>

/* local includes */
#include <support.h>

#define HELP_HEADER "tbc-mod-gen: module generator for Turbulence BEEP server \n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

const char * out_dir = NULL;

/** 
 * @brief Allows to get output directory to be used, appending this
 * value to files that are created.
 * 
 * 
 * @return Return a constant value representing the output dir.
 */
const char * get_out_dir ()
{
	/* check that the output dir is null (not initialized) */
	if (out_dir == NULL) {
		/* get default directory */
		out_dir = exarg_is_defined ("out-dir") ? exarg_get_string ("out-dir") : "./";
		
		/* check if the file has a directory separator at the
		 * end of the file */
		if (axl_cmp (out_dir + strlen (out_dir) - 1, VORTEX_FILE_SEPARATOR)) 
			out_dir = axl_strdup (out_dir);
		else
			out_dir = axl_strdup_printf ("%s%s", out_dir, VORTEX_FILE_SEPARATOR);

	} /* end if */

	/* return current dir */
	return out_dir;
}

bool tbc_mod_gen_template_create ()
{
	const char * outdir = NULL;
	axlDoc     * doc;
	axlNode    * node;
	axlNode    * nodeAux;
	
	msg ("producing a template definition at the location provided");
	
	/* get out dir */
	outdir  = get_out_dir ();
	doc     = axl_doc_create (NULL, NULL, true);
	
	node    = axl_node_create ("mod-def");
	axl_doc_set_root (doc, node);

	nodeAux = axl_node_create ("name");
	axl_node_set_child (node, nodeAux);
	axl_node_set_content (nodeAux, "mod-template", -1);
	
	nodeAux = axl_node_create ("description");
	axl_node_set_child (node, nodeAux);
	axl_node_set_content (nodeAux, "Place here a generic module description", -1);
	
	nodeAux = axl_node_create ("source-code");
	axl_node_set_child (node, nodeAux);
	
	/* update parent */
	node    = nodeAux;

	/* init node */
	axl_node_set_comment (node, "init method, called once the module is loaded", -1);
	nodeAux = axl_node_create ("init");
	axl_node_set_child (node, nodeAux);
	axl_node_set_cdata_content (nodeAux, "/* Place here your mod init code. This will be called once turbulence decides to include the module. */", -1);
	
	/* close node */
	axl_node_set_comment (node, "close method, called once the module is going to be stoped", -1);
	nodeAux = axl_node_create ("close");
	axl_node_set_child (node, nodeAux);
	axl_node_set_cdata_content (nodeAux, "/* Place here the code required to stop and dealloc resources used by your module */", -1);

	/* reconf node */
	axl_node_set_comment (node, "reconf method, called once it is received a 'reconfiguration signal'", -1);
	nodeAux = axl_node_create ("reconf");
	axl_node_set_child (node, nodeAux);
	axl_node_set_cdata_content (nodeAux, "/* Place here all your optional reconf code if the HUP signal is received */", -1);

	/* dump the xml document */
	support_dump_file (doc, 3, "%stemplate.xml", get_out_dir ());

	/* free document */
	axl_doc_free (doc);

	msg ("template created: OK");
	
	return true;
}

bool tbc_mod_gen_compile ()
{
	axlDtd   * dtd;
	axlDoc   * doc;
	axlError * error;
	axlNode  * node;
	char     * file;
	char     * mod_name;
	char     * tolower;
	char     * description;

	/* configure lookup domain for tbc-mod-gen settings */
	vortex_support_add_domain_search_path_ref (axl_strdup ("tbc-mod-gen"), 
						   vortex_support_build_filename (DATADIR, "turbulence", NULL));
	/* now find the file */
	file = vortex_support_domain_find_data_file ("tbc-mod-gen", "tbc-mod-gen.dtd");

	if (file == NULL) {
		error ("Installation problem, unable to find dtd definition file: tbc-mod-gen.dtd");
		return false;
	}

	/* parse DTD document */
	dtd = axl_dtd_parse_from_file (file, &error);
	axl_free (file);
	if (dtd == NULL) {
		/* report error and dealloc resources */
		error ("Failed to parse DTD file: %s, error found: %s", axl_error_get (error));
		axl_error_free (error);

		return false;
	} /* end if */
	
	/* nice, now parse the xml file */
	doc = axl_doc_parse_from_file (exarg_get_string ("compile"), &error);
	if (doc == NULL) {
		/* report error and dealloc resources */
		error ("unable to parse file: %s, error found: %s", 
		       exarg_get_string ("compile"),
		       axl_error_get (error));
		axl_error_free (error);
		axl_dtd_free (dtd);
	} /* end if */

	/* nice, now validate content */
	if (! axl_dtd_validate (doc, dtd, &error)) {
		/* report error and dealloc */
		error ("failed to validate module description provided: %s, error found: %s",
		       exarg_get_string ("compile"),
		       axl_error_get (error));
		axl_error_free (error);
		axl_doc_free (doc);
		axl_dtd_free (dtd);
		return false;
	} /* end if */

	/* ok, now produce source code  */
	axl_dtd_free (dtd);

	/* open file */
	node     = axl_doc_get (doc, "/mod-def/name");
	mod_name = (char *) axl_node_get_content (node, NULL);
	mod_name = support_clean_name (mod_name);
	tolower  = support_to_lower (mod_name);
	support_open_file ("%s%s.c", get_out_dir (), mod_name);

	/* place copyright if found */
	node     = axl_doc_get (doc, "/mod-def/copyright");
	if (node != NULL) {
		/* place the copyright */
	}  /* end if */
	
	write ("/* %s implementation */\n", mod_name);
	write ("#include <turbulence.h>\n\n");

	write ("/* use this declarations to avoid c++ compilers to mangle exported\n");
	write (" * names. */\n");
	write ("BEGIN_C_DECLS\n\n");

	/* init handler */
	write ("/* %s init handler */\n", mod_name);
	write ("static bool %s_init ()\n", tolower);
	node = axl_doc_get (doc, "/mod-def/source-code/init");
	if (axl_node_get_content (node, NULL)) {
		/* write the content defined */
		write ("%s\n", axl_node_get_content (node, NULL));
	}
	write ("} /* end %s_init */\n\n", tolower);

	/* close handler */
	write ("/* %s close handler */\n", mod_name);
	write ("static bool %s_close ()\n", tolower);
	node = axl_doc_get (doc, "/mod-def/source-code/close");
	if (axl_node_get_content (node, NULL)) {
		/* write the content defined */
		write ("%s\n", axl_node_get_content (node, NULL));
	}
	write ("} /* end %s_close */\n\n", tolower);

	/* reconf handler */
	write ("/* %s reconf handler */\n", mod_name);
	write ("static bool %s_reconf ()\n", tolower);
	node = axl_doc_get (doc, "/mod-def/source-code/reconf");
	if (axl_node_get_content (node, NULL)) {
		/* write the content defined */
		write ("%s\n", axl_node_get_content (node, NULL));
	}
	write ("} /* end %s_reconf */\n\n", tolower);

	/* write handler description */
	write ("/* Entry point definition for all handlers included in this module */\n");
	write ("TurbulenceModDef module_def = {\n");

	push_indent ();

	write ("\"%s\",\n", mod_name);
	node        = axl_doc_get (doc, "/mod-def/description");
	description = (char *) axl_node_get_content (node, NULL);
	write ("\"%s\",\n", description ? description : "");
	write ("%s_init,\n", tolower);
	write ("%s_close,\n", tolower);
	write ("%s_reconf,\n", tolower);

	pop_indent ();

	write ("};\n\n");

	write ("END_C_DECLS\n\n");
	
	/* close content */
	support_close_file ();

	msg ("%s created!", mod_name);

	/* dealloc */
	axl_free (mod_name);
	axl_free (tolower);
	axl_doc_free (doc);

	return true;
}


int main (int argc, char ** argv)
{
	/* install headers for help */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);

	/* install exarg options */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Provides tool version");

	exarg_install_arg ("template", "t", EXARG_NONE,
			   "Produce a module xml template. Use this file as starting point to create a module. Then use --compile.");

	exarg_install_arg ("compile", "c", EXARG_STRING,
			   "Produces the source code associated to a module, using as description the xml module definition provided.");
	
	exarg_install_arg ("out-dir", "o", EXARG_STRING,
			   "Allows to configure the out put dir to be used. If not provided, '.' will be used as default value");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* enable console log */
	turbulence_set_console_debug (true);

	/* check version argument */
	if (exarg_is_defined ("version")) {
		msg ("tbc-mod-gen version: %s", VERSION);
		return 0;
	} else if (exarg_is_defined ("template")) {
		/* produce a template definition */
		return tbc_mod_gen_template_create ();
	} else if (exarg_is_defined ("compile")) {
		/* compile template provided */
		return tbc_mod_gen_compile ();
	}

	/* no argument was produced */
	error ("no argument was provided, try to use %s --help", argv[0]);

	return 0;
}

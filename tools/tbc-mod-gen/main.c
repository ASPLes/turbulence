/*  tbc-mod-gen: A tool to produce modules for the Turbulence BEEP server
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
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

/* include axl support */
#include <axl.h>

/* include exarg support */
#include <exarg.h>

/* include turbulence */
#include <turbulence.h>

/* local includes */
#include <support.h>

/* include local dtd */
#include <tbc-mod-gen.dtd.h>

#define HELP_HEADER "tbc-mod-gen: module generator for Turbulence BEEP server \n\
Copyright (C) 2007  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER "\n\
If you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

char          * out_dir    = NULL;
TurbulenceCtx * ctx        = NULL;
VortexCtx     * vortex_ctx = NULL;

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
	axl_node_set_cdata_content (nodeAux, "/* Place here your mod init code. This will be called once turbulence decides to include the module. */\nreturn true;", -1);
	
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
	support_dump_file (ctx, doc, 3, "%stemplate.xml", get_out_dir ());

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
	axlNode  * moddef;
	char     * mod_name;
	char     * tolower;
	char     * toupper;
	char     * description;

	/* parse DTD document */
	dtd = axl_dtd_parse (TBC_MOD_GEN_DTD, -1, &error);
	if (dtd == NULL) {
		/* report error and dealloc resources */
		error ("Failed to parse DTD, error found: %s", axl_error_get (error));
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
	moddef   = axl_doc_get_root (doc);
	node     = axl_doc_get (doc, "/mod-def/name");
	mod_name = (char *) axl_node_get_content (node, NULL);
	mod_name = support_clean_name (mod_name);
	tolower  = support_to_lower (mod_name);
	toupper  = support_to_upper (mod_name);

	/* out dir */
	support_open_file (ctx, "%s%s.c", get_out_dir (), mod_name);

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

	write ("/* global turbulence context reference */\n");
	write ("TurbulenceCtx * ctx = NULL;\n\n");

	/* place here additional content */
	node    = axl_doc_get (doc, "/mod-def/source-code/additional-content");
	if (node != NULL) {
		write ("%s\n", axl_node_get_content (node, NULL));
	} /* end if */

	/* init handler */
	write ("/* %s init handler */\n", mod_name);
	write ("static bool %s_init (TurbulenceCtx * _ctx) {\n", tolower);

	push_indent ();
	write ("/* configure the module */\n");
	write ("TBC_MOD_PREPARE (_ctx);\n\n");
	pop_indent ();

	node = axl_doc_get (doc, "/mod-def/source-code/init");
	if (axl_node_get_content (node, NULL)) {
		/* write the content defined */
		write ("%s\n", axl_node_get_content (node, NULL));
	}
	write ("} /* end %s_init */\n\n", tolower);

	/* close handler */
	write ("/* %s close handler */\n", mod_name);
	write ("static void %s_close (TurbulenceCtx * _ctx) {\n", tolower);
	node = axl_doc_get (doc, "/mod-def/source-code/close");
	if (axl_node_get_content (node, NULL)) {
		/* write the content defined */
		write ("%s\n", axl_node_get_content (node, NULL));
	}
	write ("} /* end %s_close */\n\n", tolower);

	/* reconf handler */
	write ("/* %s reconf handler */\n", mod_name);
	write ("static void %s_reconf (TurbulenceCtx * _ctx) {\n", tolower);
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
	support_close_file (ctx);

	/* create the makefile required */
	support_open_file (ctx, "%sMakefile.am", get_out_dir ());
	
	write ("# Module definition\n");
	write ("EXTRA_DIST = %s\n\n", exarg_get_string ("compile"));
	
	       
	write ("INCLUDES = -Wall -g -ansi -I $(TURBULENCE_CFLAGS) -DCOMPILATION_DATE=`date +%%s` \\\n");
	push_indent ();
	
	write ("-DVERSION=\\\"$(VERSION)\\\" \\\n");
	write ("$(AXL_CFLAGS) $(VORTEX_CFLAGS) $(EXARG_CFLAGS)\n\n");
	pop_indent ();
	
	write ("# configure module binary\n");
	write ("lib_LTLIBRARIES      = %s.la\n", mod_name);
	write ("%s_la_SOURCES  = %s.c %s\n", mod_name, mod_name,
	       HAS_ATTR (moddef, "sources") ? ATTR_VALUE (moddef, "sources") : "");
	write ("%s_la_LDFLAGS  = -module -ldl\n\n", mod_name);
	
	write ("# reconfigure module installation directory\n");
	write ("libdir = `turbulence-config --mod-dir`\n\n");
	
	write ("# configure site module installation\n");
	write ("modconfdir   = `turbulence-config --mod-xml`\n");
	write ("modconf_DATA = %s.xml %s.win32.xml\n\n", mod_name, mod_name);
	
	write ("%s.xml %s.win32.xml:\n", mod_name, mod_name);
	push_indent ();
	write ("echo \"<mod-turbulence location=\\\"`turbulence-config --mod-dir`/%s.so\\\"/>\" > %s.xml\n", mod_name, mod_name);
	write ("echo \"<mod-turbulence location=\\\"../modules/%s.dll\\\"/>\" > %s.win32.xml\n", mod_name);
	pop_indent ();

	support_close_file (ctx);

	/* create autoconf if defined */
	if (exarg_is_defined ("enable-autoconf")) {

		msg ("found autoconf support files request..");
		
		/* create the autogen.sh */
		support_open_file (ctx, "%sautogen.sh", get_out_dir ());

		write ("# autogen.sh file created by tbc-mod-gen\n");
		write ("PACKAGE=\"%s: %s\"\n\n", mod_name, description);

		write ("(automake --version) < /dev/null > /dev/null 2>&1 || {\n");

		push_indent ();
		write ("echo;\n");
		write ("echo \"You must have automake installed to compile $PACKAGE\";\n");
		write ("echo;\n");
		write ("exit;\n");
		pop_indent ();

		write ("}\n\n");

		write ("(autoconf --version) < /dev/null > /dev/null 2>&1 || {\n");
		push_indent ();
		write ("echo;\n");
		write ("echo \"You must have autoconf installed to compile $PACKAGE\";\n");
		write ("echo;\n");
		write ("exit;\n");
		pop_indent ();
		write ("}\n\n");

		write ("echo \"Generating configuration files for $PACKAGE, please wait....\"\n");
		write ("echo;\n\n");

		write ("touch NEWS README AUTHORS ChangeLog\n");
		write ("libtoolize --force;\n");
		write ("aclocal $ACLOCAL_FLAGS;\n");
		write ("autoheader;\n");
		write ("automake --add-missing;\n");
		write ("autoconf;\n\n");

		write ("./configure $@ --enable-maintainer-mode --enable-compile-warnings\n");

		support_close_file (ctx);

		support_make_executable (ctx, "%sautogen.sh", get_out_dir ());

		/* now create the configure.ac file */

		support_open_file (ctx, "%sconfigure.ac", get_out_dir ());

		write ("dnl configure.ac template file created by tbc-mod-gen\n");
		write ("AC_INIT(%s.c)\n\n", mod_name);

		write ("dnl declare a global version value\n");
		write ("%s_VERSION=\"0.0.1\"\n", toupper);
		write ("AC_SUBST(%s_VERSION)\n\n", toupper);

		write ("AC_CONFIG_AUX_DIR(.)\n");
		write ("AM_INIT_AUTOMAKE(%s, $%s_VERSION)\n\n", mod_name, toupper);

		write ("AC_CONFIG_HEADER(config.h)\n");
		write ("AM_MAINTAINER_MODE\n");
		write ("AC_PROG_CC\n");
		write ("AC_ISC_POSIX\n");
		write ("AC_HEADER_STDC\n");
		write ("AM_PROG_LIBTOOL\n\n");

		write ("dnl external dependencies\n");
		write ("PKG_CHECK_MODULES(AXL, axl >= %s)\n\n", AXL_VERSION);

		write ("dnl general libries subsitution\n");
		write ("AC_SUBST(AXL_CFLAGS)\n");
		write ("AC_SUBST(AXL_LIBS)\n\n");

		write ("dnl external dependencies\n");
		write ("PKG_CHECK_MODULES(VORTEX, vortex >= %s) \n\n", VORTEX_VERSION);

		write ("dnl general libries subsitution\n");
		write ("AC_SUBST(VORTEX_CFLAGS)\n");
		write ("AC_SUBST(VORTEX_LIBS)\n\n");

		write ("dnl external dependencies\n");
		write ("PKG_CHECK_MODULES(EXARG, exarg)\n\n");

		write ("dnl general libries subsitution\n");
		write ("AC_SUBST(EXARG_CFLAGS)\n");
		write ("AC_SUBST(EXARG_LIBS)\n\n");
		
		write ("dnl external dependencies\n");
		write ("PKG_CHECK_MODULES(TURBULENCE, turbulence >= %s)\n\n", VERSION);

		write ("dnl general libries subsitution\n");
		write ("AC_SUBST(TURBULENCE_CFLAGS)\n");
		write ("AC_SUBST(TURBULENCE_LIBS)\n\n");

		write ("AC_OUTPUT([\n");
		write ("Makefile\n");
		write ("])\n\n");

		write ("echo \"------------------------------------------\"\n");
		write ("echo \"--       mod_template Settings          --\"\n");
		write ("echo \"------------------------------------------\"\n");
		write ("echo \"------------------------------------------\"\n");
		write ("echo \"--            Let it BEEP!              --\"\n");
		write ("echo \"--                                      --\"\n");
		write ("echo \"--     NOW TYPE: make; make install     --\"\n");
		write ("echo \"------------------------------------------\"\n");

		support_close_file (ctx);
		
	} /* end if */

	/* dealloc */
	axl_free (tolower);
	axl_free (toupper);
	axl_doc_free (doc);

	/* create the script file */
	support_open_file (ctx, "%sgen-code", get_out_dir ());

	write ("#!/bin/sh\n\n");

	/* write the mod gen */
	write ("tbc-mod-gen --compile %s --out-dir %s\n", exarg_get_string ("compile"), exarg_get_string ("out-dir"));
	
	support_close_file (ctx);

	support_make_executable (ctx, "%sgen-code", get_out_dir ());

	msg ("%s created!", mod_name);
	axl_free (mod_name);

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

	exarg_install_arg ("compile", "p", EXARG_STRING,
			   "Produces the source code associated to a module, using as description the xml module definition provided.");
	
	exarg_install_arg ("out-dir", "o", EXARG_STRING,
			   "Allows to configure the out put dir to be used. If not provided, '.' will be used as default value");

	exarg_install_arg ("enable-autoconf", "e", EXARG_NONE,
			   "Makes the output to produce autoconf support files: autogen.sh and configure.ac. It provides an starting point to build and compile your module.");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Increase the level of log to console produced.");

	exarg_install_arg ("debug3", NULL, EXARG_NONE,
			   "Makes logs produced to console to inclue more information about the place it was launched.");

	exarg_install_arg ("color-debug", "c", EXARG_NONE,
			   "Makes console log to be colorified.");	

	/* call to parse arguments */
	exarg_parse (argc, argv);

	ctx = turbulence_ctx_new ();

	/* create a vortex context and init the support module */
	vortex_ctx = vortex_ctx_new ();
	turbulence_ctx_set_vortex_ctx (ctx, vortex_ctx);
	vortex_support_init (vortex_ctx);

	/* configure context debug according to values received */
	turbulence_log_enable  (ctx, true);
	turbulence_log2_enable (ctx, exarg_is_defined ("debug2"));
	turbulence_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* check console color debug */
	turbulence_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* check version argument */
	if (exarg_is_defined ("version")) {
		msg ("tbc-mod-gen version: %s", VERSION);
		goto finish;
	} else if (exarg_is_defined ("template")) {
		/* produce a template definition */
		tbc_mod_gen_template_create ();
	} else if (exarg_is_defined ("compile")) {
		/* compile template provided */
		tbc_mod_gen_compile ();
	} else {

		/* no argument was produced */
		error ("no argument was provided, try to use %s --help", argv[0]);
	}

 finish:
	/* terminate exarg */
	exarg_end ();

	/* cleanup support module */
	vortex_support_cleanup (vortex_ctx);
	axl_free (out_dir);

	/* free context */
	turbulence_ctx_free (ctx);
	vortex_ctx_free (vortex_ctx);

	return 0;
}

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
 *         C/ Dr. Michavila N� 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

#define MOD_TEST_URI1 "http://turbulence.ws/profiles/test1"
#define MOD_TEST_URI2 "http://turbulence.ws/profiles/test2"
#define MOD_TEST_URI3 "http://turbulence.ws/profiles/test3"
#define MOD_TEST_URI4 "http://turbulence.ws/profiles/test4"

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other task. The function must
 * return true to signal that the module was initialized
 * ok. Otherwise, false must be returned.
 */
static bool test_init ()
{
	msg ("Turbulence BEEP server, test module: init");

	/* register all profiles without handlers: testing purposes */
	vortex_profiles_register (MOD_TEST_URI1,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (MOD_TEST_URI2,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (MOD_TEST_URI3,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (MOD_TEST_URI4,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	return true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void test_close ()
{
	msg ("Turbulence BEEP server, test module: close");
}

/**
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-test",
	"Turbulence BEEP server, test module",
	test_init,
	test_close
};

END_C_DECLS

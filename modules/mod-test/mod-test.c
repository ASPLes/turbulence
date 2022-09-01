/*  Turbulence BEEP application server
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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
#include <turbulence.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

#define MOD_TEST_URI1 "http://turbulence.ws/profiles/test1"
#define MOD_TEST_URI2 "http://turbulence.ws/profiles/test2"
#define MOD_TEST_URI3 "http://turbulence.ws/profiles/test3"
#define MOD_TEST_URI4 "http://turbulence.ws/profiles/test4"

TurbulenceCtx * ctx = NULL;

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure Vortex, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  test_init (TurbulenceCtx * _ctx)
{
	/* configure the module */
	TBC_MOD_PREPARE (_ctx);

	/* using TBC_VORTEX_CTX allows to get the vortex context from
	 * the turbulence ctx */
	
	msg ("Turbulence BEEP server, test module: init");

	/* register all profiles without handlers: testing purposes */
	vortex_profiles_register (TBC_VORTEX_CTX (ctx),
				  MOD_TEST_URI1,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (TBC_VORTEX_CTX (ctx),
				  MOD_TEST_URI2,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (TBC_VORTEX_CTX (ctx),
				  MOD_TEST_URI3,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	vortex_profiles_register (TBC_VORTEX_CTX (ctx),
				  MOD_TEST_URI4,
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	return axl_true;
}

/** 
 * @brief Close function called once the turbulence server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void test_close (TurbulenceCtx * ctx)
{
	msg ("Turbulence BEEP server, test module: close");
	return;
}

/** 
 * @brief The reconf function is used by turbulence to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void test_reconf (TurbulenceCtx * ctx) {
	msg ("Turbulence BEEP server configuration have change");
	return;
}

/** 
 * @brief The ppath_selected function is used by turbulence to signal
 * modules that a connection was finally configured under the provided
 * profile path. This is important because a profile path defines how
 * the connection will be limited and configured to accept profiles,
 * configuring process permission and so on. 
 *
 * It is also useful because at the time a profile path is selected,
 * serverName name is available, allowing the module to take especial
 * actions.
 *
 * @param ctx The \ref TurbulenceCtx where the profile path was selected.
 *
 * @param ppath_selected Reference to the object representing the profile path selected. See \ref turbulence_ppath.
 *
 * @param conn The VortexConnection object that was configured with the provided profile path.
 *
 * @return axl_true to accept or not the connection. Keep in mind
 * returning axl_false may also terminate current child process
 * (according to \ref turbulence_clean_start "clean start" configuration).
 */
static axl_bool test_ppath_selected (TurbulenceCtx * ctx, TurbulencePPathDef * ppath_selected, VortexConnection * conn) {
	msg ("Turbulence configured ppath %s to connection id %d",
	     turbulence_ppath_get_name (ppath_selected), 
	     vortex_connection_get_id (conn));

	return axl_true;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the turbulence will lookup to load the rest of items.
 */
TurbulenceModDef module_def = {
	"mod-test",
	"Turbulence BEEP server, test module",
	test_init,
	test_close,
	test_reconf,
	NULL,
	test_ppath_selected
};

END_C_DECLS

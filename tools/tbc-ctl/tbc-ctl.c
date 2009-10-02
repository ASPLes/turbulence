/*  tbc-ctl: A command line tool to manage turbulence remote management interface
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
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

/* include sasl support */
#include <vortex_sasl.h>

/* include read line support */
#include <readline/readline.h>

#define HELP_HEADER "tbc-ctl: a CLI tool to manage turbulence remote management interface\n\
Copyright (C) 2009  Advanced Software Production Line, S.L.\n\n"

#define POST_HEADER ""

/* " */

/** 
 * @brief Default turbulence and vortex context.
 */
TurbulenceCtx    * ctx;
VortexCtx        * vortex_ctx;
VortexConnection * conn;

/** 
 * @brief Profile that defines radmin channels
 */
#define RADMIN_URI "urn:aspl.es:beep:profiles:radmin-ctl"

#if !defined(getpass)
/* add a prototype to avoid a warning */
char *getpass( const char * prompt );
#endif 

#ifdef AXL_OS_WIN32
char     tbc_ctl_getch (void )
{

	HANDLE hInput;
	char   ch;
	int   n;

	/* get the STDIN handle. */
	hInput = GetStdHandle(STD_INPUT_HANDLE);
	
	/* turn off echoing */
        SetConsoleMode(hInput, ENABLE_PROCESSED_INPUT);
	
	/* reac one char from the console */
	ReadConsole(hInput, &ch, 1, (DWORD *) &n, NULL);

	return ch;
}
#endif

/** 
 * @brief Allows to get a password from the user writting to stdout hash simbols.
 * 
 * @param prompt The prompt to be showed to the user.
 * 
 * @return The password get. Do not free returned value. If you need a
 * newly allocated value use \ref afdal_getpass_allocated.
 */
char  * tbc_ctl_getpass (char  * prompt) 
{
#ifdef AXL_OS_UNIX
	char  * s;
#endif
#ifdef AXL_OS_WIN32	
#define MAX_LEN 30
	int character, length, maxlen = MAX_LEN;
	static char s[MAX_LEN], *position;
#endif

	/* get the password unix way */
#ifdef AXL_OS_UNIX
	s =  getpass (prompt);
#endif

	/* get the password windows way */
#ifdef AXL_OS_WIN32	
	length   = 0;
	position = s;
	fprintf (stdout, prompt);

	while ((character = tbc_ctl_getch ()) != '\r' && 
	       (character != '\n') && (character != 27) && (length <= maxlen)) { 
		if (character == '\b')	{
			if ( length > 0 ) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				--length;
				--position;
			}
		}
		else if (character < 32 || character > 127);
		else {   
			putchar('#');
			*position++ = (char) character;
			++length;
		}
	}

	*position = '\0';
	putchar ('\n');

#endif
	/* function end */
	return s;
}


axl_bool tbc_ctl_do_connection (void) {

	const char * host = "localhost";
	const char * port = "602";

	/* try first to create a connection using default values or
	   user provided values */
	if (exarg_is_defined ("host"))
		host = exarg_get_string ("host");
	if (exarg_is_defined ("port"))
		port = exarg_get_string ("port");
	
	msg ("connecting turbulence at %s:%s..", host, port);
	conn = vortex_connection_new (vortex_ctx, 
				      host, 
				      port,
				      NULL, NULL);
	/* check connection returned */
	return vortex_connection_is_ok (conn, axl_false);
}

void tbc_ctl_frame_received (VortexChannel    * channel, 
			     VortexConnection * connection,
			     VortexFrame      * frame,
			     axlPointer         user_data)
{
	/* content received */
	printf ("%s\n", (char *) vortex_frame_get_payload (frame));
	return;
}

axl_bool tbc_ctl_enable_sasl (void)
{
	char         * mech = axl_strdup (VORTEX_SASL_PLAIN);
	VortexStatus   status;
	char         * message = NULL;
	char         * string;

	/* init sasl */
	if (! vortex_sasl_init (vortex_ctx)) {
		printf ("ERROR: failed to initialize SASL module..unable to authenticate\n");
		return axl_false;
	}

	/* get plan mechamis */
	if (exarg_is_defined ("sasl-method"))
		mech = axl_strdup_printf ("%s/%s", VORTEX_SASL_FAMILY, axl_stream_to_upper (exarg_get_string ("sasl-method")));
	msg ("using SASL auth %s", mech);
	
	/* get auth properies */
	if (axl_cmp (mech, VORTEX_SASL_PLAIN)) {
		if (exarg_is_defined ("user"))
			vortex_sasl_set_propertie (conn, VORTEX_SASL_AUTH_ID, exarg_get_string ("user"), NULL);
		else {
			string = readline ("User: ");
			vortex_sasl_set_propertie (conn, VORTEX_SASL_AUTH_ID, string, axl_free);
		} /* end if */

		/* now password */
		if (exarg_is_defined ("password"))
			vortex_sasl_set_propertie (conn, VORTEX_SASL_PASSWORD, exarg_get_string ("password"), NULL);
		else {
			/* close stdout */
			string = tbc_ctl_getpass ("Password: ");
			string = axl_strdup (string);

			/* end if */
			vortex_sasl_set_propertie (conn, VORTEX_SASL_PASSWORD, string, axl_free);
		} /* end if */
	} /* end if */

	/* enable auth operation */
	vortex_sasl_start_auth_sync (conn, mech, &status, &message);

	if (status  == VortexOk) 
		msg ("SASL status: %s", message);
	else
		error ("SASL status: %s", message);

	/* free status message nad mech */
	axl_free (mech);

	/* return SASL auth status */
	return status == VortexOk;
}

/** 
 * @internal Function used to create management interface.
 */
axl_bool  tbc_ctl_create_management_channel (void) {
	VortexChannel * channel;
	char          * msg;
	int             code;

 create_channel:
	/* try first to create the channel without using SASL first */
	channel = vortex_channel_new (conn,
				      0, 
				      RADMIN_URI,
				      /* no close handler */
				      NULL, NULL, 
				      /* frame received */
				      tbc_ctl_frame_received, NULL,
				      NULL, NULL);
	if (channel == NULL) {
		error ("Failed to create channel, error was: ");
		while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
			error ("  %d: %s", code, msg);
			axl_free (msg);
		} /* end while */

		/* check if the channel is already authenticated */
		if (vortex_sasl_is_authenticated (conn))
			return axl_false;

	} else {
		/* channel created */
		return axl_true;
	} /* end if */

	/* ok, now try enable SASL auth */
	if (! tbc_ctl_enable_sasl ()) 
		return axl_false;
	
	/* try again to create the channel */
	goto create_channel;
	return axl_false;
}

void tbc_ctl_command_loop (void)
{
	char * command;
	char * prompt = axl_strdup_printf ("tbc-ctl:%s:%s> ", 
					   vortex_connection_get_host (conn), vortex_connection_get_port (conn));

	while (axl_true) {
		/* read command */
		command = readline (prompt);

		/* process command */
		if (command == NULL || axl_cmp (command, "quit") || axl_cmp (command, "exit")) {
			axl_free (command);
			msg ("Exiting..");
			break;
		}
		
		/* free command */
		axl_free (command);
	} /* end if */

	/* terminate prompt */
	axl_free (prompt);

	return;
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

	exarg_install_arg ("host", "h", EXARG_STRING, 
			   "Configures turbulence host location (default localhost)");

	exarg_install_arg ("port", "p", EXARG_STRING, 
			   "Configures turbulence port location (default 602)");

	exarg_install_arg ("sasl-method", "s", EXARG_STRING, 
			   "Optionally configures default SASL authentication method to be used (default PLAIN if --user is used)");

	exarg_install_arg ("user", "u", EXARG_STRING, 
			   "Optionally configures SASL authentication user");

	exarg_install_arg ("debug", "d", EXARG_NONE,
			   "Allows to configure to enable log");

	exarg_install_arg ("debug2", NULL, EXARG_NONE,
			   "Allows to configure to enable second level log");

	exarg_install_arg ("color-debug", NULL, EXARG_NONE,
			   "Allows to configure color debug");

	/* call to parse arguments */
	exarg_parse (argc, argv);

	/* init turbulence context */
	ctx        = turbulence_ctx_new ();
	vortex_ctx = vortex_ctx_new ();

	/* init vortex */
	if (! vortex_init_ctx (vortex_ctx))  {
		error ("Unable to initialize vortex context.."); 
		return -1;
	}
	
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

	/* init application */
	if (! tbc_ctl_do_connection ()) {
		
		error ("Failed to perform connection application: %s..",
		       vortex_connection_get_message (conn));
		vortex_connection_close (conn);
		return -1;
	}
	msg ("connected OK!");

	/* create tbc-ctl management channel */
	if (! tbc_ctl_create_management_channel ()) {
		vortex_connection_close (conn);
		return -1;
	} /* end if */

	/* ok, we are now connected, loop reading user commands */
	tbc_ctl_command_loop ();

	/* free context */
	turbulence_ctx_free (ctx);

	/* finish vortex */
	vortex_exit_ctx (vortex_ctx, axl_true);

	return 0;
}

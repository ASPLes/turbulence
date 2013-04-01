#include <vortex.h>
#include <vortex_websocket.h>

#define SIMPLE_CHANNEL_CREATE(uri) vortex_channel_new (conn, 0, uri, NULL, NULL, NULL, NULL, NULL, NULL)

int main (int argc, char ** argv) {
	VortexCtx            * vCtx;
	VortexConnection     * conn;
	VortexChannel        * channel;
	VortexWebsocketSetup * wss_setup;
	const char           * serverName = "test-25.server.nochild";
	VortexFrame          * frame;
	VortexAsyncQueue     * queue;

	if (argc > 1)
		serverName = argv[1];

	printf ("Test 25: running test with serverName: %s\n", serverName);

	/* init vortex here */
	vCtx = vortex_ctx_new ();
	if (! vortex_init_ctx (vCtx)) {
		printf ("Test 00-a: failed to init VortexCtx reference..\n");
		return axl_false;
	}

	/* create websocket TLS connection */
	wss_setup = vortex_websocket_setup_new (vCtx);
	/* setup we want TLS enabled */
	vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_TLS, INT_TO_PTR (axl_true));
	/* setup Host: header to indicate the profile path or application we want */
	vortex_websocket_setup_conf (wss_setup, VORTEX_WEBSOCKET_CONF_ITEM_HOST, (axlPointer) serverName);

	/* create Websocket connection */
	printf ("Test 25: Creating websocket connection (Host: %s): 127.0.0.1:1602\n", serverName);
	conn = vortex_websocket_connection_new ("127.0.0.1", "1602", wss_setup, NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection creation (%d, %s)\n", 
			vortex_connection_get_status (conn), vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */
	printf ("Test 25: connection created, now open a channel\n");
	channel = SIMPLE_CHANNEL_CREATE ("urn:aspl.es:beep:profiles:reg-test:profile-22:1");
	if (channel == NULL) {
		printf ("ERROR (2.2): expected to find proper channel creation but a proper reference was found..\n");
		return axl_false;
	} /* end if */

	/* send some content */
	printf ("Test 25: sending message..\n");
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	if (! vortex_channel_send_msg (channel, "this is a test message..", 24, 0)) {
		printf ("ERROR (2.3): expected to be able to send test message..\n");
		return axl_false;
	} /* end if */

	/* wait for the reply */
	printf ("Test 25: message sent, now waiting for message..\n");
	frame = vortex_async_queue_pop (queue);
	if (frame == NULL) {
		printf ("ERROR (2.4): expected to receive frame reference..\n");
		return axl_false;
	} /* end if */
	

	printf ("Test 25: reply received, checking it is what we expected..\n");
	if (! axl_cmp (vortex_frame_get_payload (frame), "this is a test message..")) {
		printf ("ERROR (2.5): expected to receive different content..\n");
		return axl_false;
	} /* end if */

	printf ("Test 25: Test ok..\n");

	/* close connection */
	vortex_connection_close (conn);

	vortex_async_queue_unref (queue);

	vortex_exit_ctx (vCtx, axl_true);
	

	return 0;
}

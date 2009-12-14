# import beep services
import vortex
import vortex.tls
import vortex.sasl

URI = "urn:aspl.es:beep:profiles:python-test"

def frame_received (conn, channel, frame, xml_conf):

    # reply acknoledge
    channel.send_rpy ("", 0, frame.msg_no)
    return

def app_init (_tbc):
    global tbc;

    # register profiles (agent notification)
    vortex.register_profile (tbc.vortex_ctx, URN,
                             frame_received=frame_received)

    # return initialization ok
    tbc.msg ("Test 13: python-test initialized")
    return True

def application_close ():
    pass

def application_reload ():
    pass


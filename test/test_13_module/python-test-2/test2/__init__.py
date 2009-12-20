# import beep services
import vortex
import vortex.tls
import vortex.sasl

URN = "urn:aspl.es:beep:profiles:python-test-2"

# Turbulence context (PyTurbulenceCtx)
tbc = None

def frame_received (conn, channel, frame, xml_conf):

    tbc.msg ("Test 13: python-test-2 received content: '" + frame.payload + "'")

    if frame.payload == "python-check-2":
        channel.send_rpy ("hey, this is python app 2", 25, frame.msg_no)
        return

    # reply acknoledge
    channel.send_rpy ("Expected to find different content but found this: " + frame.payload, 51 + len (frame.payload), frame.msg_no)
    return

def app_init (_tbc):
    # acquire a reference to the turbulence context
    global tbc;
    tbc = _tbc;

    # register profiles (agent notification)
    vortex.register_profile (tbc.vortex_ctx, URN,
                             frame_received=frame_received)

    # return initialization ok
    tbc.msg ("Test 13: python-test-2 initialized")
    return True

def app_close ():
    pass

def app_reload ():
    pass


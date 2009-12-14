# import beep services
import vortex
import vortex.tls
import vortex.sasl

URN = "urn:aspl.es:beep:profiles:python-test"

# Turbulence context (PyTurbulenceCtx)
tbc = None

def frame_received (conn, channel, frame, xml_conf):

    if frame.content == "python-check":
        channel.send_rpy ("hey, this is python app 1", 25, frame.msg_no)
        return

    # reply acknoledge
    channel.send_rpy ("Expected to find different content but found this: " + frame.content, 51 + len (frame.content), frame.msg_no)
    return

def app_init (_tbc):
    # acquire a reference to the turbulence context
    global tbc;
    tbc = _tbc;

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


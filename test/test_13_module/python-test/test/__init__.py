# import beep services
import vortex
import vortex.tls
import vortex.sasl

URN = "urn:aspl.es:beep:profiles:python-test"

# Turbulence context (PyTurbulenceCtx)
tbc = None

def filter_msg (conn, data):

    tbc.msg ("Test 13-b: checking to filter connection id=%d with %s" % (conn.id, str (data)))
    # filter particular connection
    if conn.id == data:
        return True

    # do not filter
    return False

def frame_received (conn, channel, frame, xml_conf):

    tbc.msg ("Test 13: python-test received content: '%s' over connection id %d" % (frame.payload, conn.id))

    if frame.payload == "python-check":
        channel.send_rpy ("hey, this is python app 1", 25, frame.msg_no)
        return

    if frame.payload == "broadcast 1":
        channel.send_rpy ("hey, broadcast 1", 16, frame.msg_no)
        tbc.broadcast_msg ("This should not reach", -1, "urn:aspl.es:beep:profiles:python-test", filter_msg, conn.id)
        return

    if frame.payload == "broadcast 2":
        channel.send_rpy ("hey, broadcast 2", 16, frame.msg_no)
        tbc.broadcast_msg ("This should reach", -1, "urn:aspl.es:beep:profiles:python-test", filter_msg, None)
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
    tbc.msg ("Test 13: python-test initialized")
    return True

def app_close ():
    pass

def app_reload ():
    pass


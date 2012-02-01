#!/usr/bin/python

import os
import commands
import termios
import sys

def enable_echo(fd, enabled):
    (iflag, oflag, cflag, lflag, ispeed, ospeed, cc) = termios.tcgetattr (fd)
    if enabled:
        lflag |= termios.ECHO
    else:
        lflag &= ~termios.ECHO
    new_attr = [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]
    termios.tcsetattr(fd, termios.TCSANOW, new_attr)
    return

def gen_password (passvalue):
    import hashlib
    value = hashlib.md5 (passvalue).hexdigest ().upper()

    hash_len = len (value) * 1.5 -1
    iterator = 2
    while iterator < hash_len:
        value = value[:iterator] + ":" + value[iterator:]
        iterator += 3
    
    return value

# track if we should reboot
should_reboot = False

if os.geteuid () != 0:
    print ("ERROR: only root can run this tool")
    sys.exit (-1)

# get base instalation
(status, output) = commands.getstatusoutput ("turbulence --conf-location | grep SYSCONFDIR")
if status:
    print ("ERROR: turbulence get location command failed with status %d, error: %s" % (status, output))
    sys.exit (-1)

output      = output.replace ("SYSCONFDIR:", "")
baseconfdir = output.replace (" ", "")

print ("INFO: found base configuration at %s/turbulence" % baseconfdir)

# check if radmin directory exists
if not os.path.exists ("%s/turbulence/radmin" % baseconfdir):
    print ("INFO: creating directory: %s/turbulence/radmin" % baseconfdir)
    os.mkdir ("%s/turbulence/radmin" % baseconfdir)

# ensure permissions are right
(status, output) = commands.getstatusoutput ("chmod o-rwx %s/turbulence/radmin" % baseconfdir)
if status:
    print ("ERROR: failed to ensure permissions inside %s/turbulence/radmin directory" % baseconfdir)
    sys.exit (-1)

# create radmin.conf
if not os.path.exists ("%s/turbulence/profile.d/radmin.conf" % baseconfdir):
    import hashlib
    import random

    # build serverName 
    serverName = hashlib.md5 (str(random.random ())).hexdigest ()
    
    print ("INFO: creating %s/turbulence/profile.d/radmin.conf" % baseconfdir)
    open ("%s/turbulence/profile.d/radmin.conf" % baseconfdir, "w").write ("<!-- profile path to load mod-radmin from localhost -->\n\
<path-def server-name='%s' \n\
          src='127.0.0.1' \n\
          path-name='local radmin' \n\
          work-dir='%s/turbulence/radmin'>\n\
    <if-success profile='http://iana.org/beep/SASL/.*' connmark='sasl:is:authenticated' >\n\
        <allow profile='urn:aspl.es:beep:profiles:radmin-ctl' />\n\
     </if-success>\n\
</path-def>" % (serverName, baseconfdir))

    # ensure permissions are right
    (status, output) = commands.getstatusoutput ("chmod o-rwx %s/turbulence/profile.d/radmin.conf" % baseconfdir)
    if status:
        print ("ERROR: failed to ensure permissions for %s/turbulence/profile.d/radmin.conf file" % baseconfdir)
        sys.exit (-1)

    # flag we have to reboot
    should_reboot = True

# create sasl.conf
if not os.path.exists ("%s/turbulence/radmin/sasl.conf" % baseconfdir):
    print ("INFO: creating %s/turbulence/radmin/sasl.conf" % baseconfdir)
    open ("%s/turbulence/radmin/sasl.conf" % baseconfdir, "w").write ('<mod-sasl>\n\
   <auth-db remote-admins="remote-admins.xml" \n\
            remote="no" \n\
            format="md5" \n\
            location="auth-db.xml" \n\
            type="xml" />\n\
   <method-allowed>\n\
      <method value="plain" />\n\
   </method-allowed>\n\
   <login-options>\n\
      <max-allowed-tries value="3" action="drop"/>\n\
      <accounts-disabled action="drop" />\n\
   </login-options>\n\
</mod-sasl>')

# create auth-db.xml
if not os.path.exists ("%s/turbulence/radmin/auth-db.xml" % baseconfdir):
    print ("No database found, creating one. For this, we need a user and a password")
    user     = raw_input ("Auth login to create: " ).strip ()
    enable_echo (1, False)
    password = raw_input ("Type password: " ).strip ()
    enable_echo (1, True)
    print ""

    # gen password
    password = gen_password (password)
    
    print ("INFO: creating %s/turbulence/radmin/auth-db.xml" % baseconfdir)
    open ("%s/turbulence/radmin/auth-db.xml" % baseconfdir, "w").write ("<sasl-auth-db>\n\
    <auth user_id='%s' password='%s' disabled='no'/>\n\
</sasl-auth-db>" % (user, password))

# try to enable module if not
if not os.path.exists ("%s/turbulence/mods-enabled/mod_radmin.xml" % baseconfdir):
    print ("INFO: enabling mod-radmin module")
    (status, output) = commands.getstatusoutput ("ln -s %s/turbulence/mods-available/mod_radmin.xml %s/turbulence/mods-enabled/mod_radmin.xml" % (baseconfdir, baseconfdir))
    if status:
        print ("INFO: failed to enable module, ln command failed: %s" % output)
        sys.exit (-1)

    # flag you should reboot
    should_reboot = True

print ("INFO: configuration done!")

if should_reboot:
    print ("INFO: you must reboot your turbulence server to make changes effective")



    


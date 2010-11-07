#!/usr/bin/python

import sys

if len(sys.argv) != 3:
    print "Please provide a password and hashing function: \n    Use: " + sys.argv[0] + " [md5|sha1] password"
    sys.exit (0)

# get md5 password
if sys.argv[1] == "md5":
    import md5
    value = md5.new (sys.argv[2]).hexdigest ().upper()
elif sys.argv[1] == "sha1":
    import sha
    value = sha.new (sys.argv[2]).hexdigest ().upper()
else:
    print "ERROR: unsupported hash: " + sys.argv[2]
    sys.exit (-1)

# now we have to insert : every two chars
iterator = 2

# hash len is the current length of the hash + every : placed. Because
# it is placed every two positions we multiply the length by 1,5 (-1)
# to avoid adding : at the end.
hash_len = len (value) * 1.5 -1
while iterator < hash_len:
    value = value[:iterator] + ":" + value[iterator:]
    iterator += 3

# print the password
print "The password prepared is: " + value

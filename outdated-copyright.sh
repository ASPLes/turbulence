#!/bin/bash

rm -rf doc/html
rm -rf debian/tmp
rm -rf debian/libturbulence-dev
rm -rf debian/libturbulence-mod-sasl
rm -rf debian/libturbulence-mod-websocket
rm -rf debian/libturbulence-mod-radmin
rm -rf debian/libturbulence-mod-sasl-mysql
rm -rf debian/turbulence-utils
rm -rf debian/libturbulence
rm -rf debian/libturbulence-mod-tls
rm -rf debian/libturbulence-mod-python
rm -rf debian/libturbulence-mod-tunnel
rm -rf debian/turbulence-server

# find all files that have copy right declaration associated to Aspl that don't have 
# the following declaration year
current_year="2025"
LANG=C rgrep "Copyright" data debian-files/ doc modules rpm src test tools TODO  turbulence.pc.in configure.ac web 2>&1 | grep "Advanced" | grep -v "Permission denied" | grep -v '~:' | grep -v '/\.svn/' | grep -v "${current_year}"

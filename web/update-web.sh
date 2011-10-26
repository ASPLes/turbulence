#!/bin/bash

echo "Updating mod-test.c.html"
(cd ../modules/mod-test; c2html mod-test.c; cp mod-test.c.html ../../web/)
(cp ../modules/mod-test/mod-test.c .)
(cp ../modules/mod-test/Makefile.am .)
(cp ../modules/mod-test/mod-test.xml.in .)

read -p "Update html files (y/n): " html
if [ $html == "y" ]; then
	rsync -avz turbulence.css mod-test.c Makefile.am mod-test.xml.in mod-test.c.html *.html aspl-web@www.aspl.es:www/turbulence
fi

read -p "Update image files (y/n): " images
if [ $images == "y" ]; then
	rsync -avz images/*.png images/*.gif aspl-web@www.aspl.es:www/turbulence/images/
fi


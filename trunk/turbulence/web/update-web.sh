#!/bin/bash

echo "Updating mod-test.c.html"
(cd ../modules/mod-test; c2html mod-test.c; cp mod-test.c.html ../../web/)
(cp ../modules/mod-test/mod-test.c .)
(cp ../modules/mod-test/Makefile.am .)
(cp ../modules/mod-test/mod-test.xml.in .)

read -p "Update html files (y/n): " html
if [ $html == "y" ]; then
	scp turbulence.css index.html install.html tunnel.html sasl.html extending.html doc.html news.html configuring.html downloads.html turbulence.html commercial.html licensing.html support.html aspl@www.aspl.es:www/turbulence
fi

read -p "Update image files (y/n): " images
if [ $images == "y" ]; then
	scp images/*.png images/*.gif aspl@www.aspl.es:www/turbulence/images/
fi


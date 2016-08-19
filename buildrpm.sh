#!/bin/sh

echo "Preparing sources"
rm -rf rpm/SOURCES/*.tar.gz
rm -rf rpm/BUILD/*
rm -rf rpm/BUILDROOT/*
test -d rpm/RPMS || mkdir -p rpm/RPMS
find rpm/RPMS/ -type f -exec rm {} \;

# refresh package
./autogen.sh --prefix=/usr --sysconfdir=/etc
make dist
cp turbulence-`cat VERSION`.tar.gz rpm/SOURCES

echo "Calling to compile packages.."
LANG=C rpmbuild -ba --define '_topdir /usr/src/turbulence/rpm' rpm/SPECS/turbulence.spec
error=$?
if [ $error != 0 ]; then
    echo "ERROR: ***"
    echo "ERROR: rpmbuild command failed, exitcode=$error"
    echo "ERROR: ***"
    exit $error
fi


echo "Output ready at rpm/RPMS"
find rpm/RPMS -type f -name '*.rpm' > rpm/RPMS/files
cat rpm/RPMS/files




#!/bin/sh
set -e
. buildbot/slave/prepare.sh

echo -n "creating ${BUILDDIR} ... "

if [ ! -d ${BUILDDIR} ] ; then
	mkdir -p ${BUILDDIR}
	echo "done."
else
	echo "skipped."
fi

echo "configuring ${BUILDDIR} with $@ ..."

cd ${BUILDDIR}
cmake ${SOURCEDIR} $@

echo "erasing old base content..."
rm -rf base

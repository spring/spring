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

echo "building in ${BUILDDIR}"
echo "configuring ${SOURCEDIR} with ${CMAKEPARAM} $@ ..."

cd ${BUILDDIR}
cmake ${CMAKEPARAM} $@ ${SOURCEDIR}

echo "erasing old base content..."
rm -rf base

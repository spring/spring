#!/bin/bash

# Quit on error.
set -e

cd $(dirname $0)/..

CONFIG=${1}
BRANCH=${2}
MINGWLIBS_PATH=${3}
MINGW_HOST=i586-mingw32msvc-
BUILDDIR=${PWD}/build-${CONFIG}
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}
REV=$(git describe --tags)

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi

#strip symbols and archive them
cd ${BUILDDIR}
for tostripfile in spring.exe unitsync.dll $(find AI/Skirmish -name SkirmishAI.dll); do
	echo ${tostripfile}
	debugfile=${tostripfile}.dbg
	${MINGW_HOST}objcopy --only-keep-debug ${tostripfile} ${debugfile}
	${MINGW_HOST}strip --strip-debug --strip-unneeded ${tostripfile}
	${MINGW_HOST}objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
done
cd $OLDPWD

# The 0 means: do not define DIST_DIR
./installer/make_installer.pl 0

mv ./installer/spring*.exe ${TMP_PATH}


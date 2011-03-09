#!/bin/bash
set -e
. buildbot/slave/prepare.sh

REMOTE_HOST=localhost
REMOTE_BASE=/home/buildbot/www
BASE_ARCHIVE="${TMP_PATH}/${VERSION}_base.7z"
MINGWLIBS_ARCHIVE="${TMP_PATH}/${VERSION}_mingwlibs.7z"
AIS_ARCHIVE="${TMP_PATH}/${VERSION}_ais.z7"
CONTENT_DIR=${SOURCEDIR}/cont
MINGWLIBS_DIR=${SOURCEDIR}/../../mingwlibs/dll
CMD="rsync -avz --chmod=D+rx,F+r"
#Ultra settings
SEVENZIP="7za u -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on"

umask 022

for i in BUILDDIR CONTENT_DIR MINGWLIBS_DIR ; do
	eval echo Using $i: \$$i
done  

# Usage: zip <file> <name>
function zip() {
	file=$1
	name=$2
	if [ -f ${file} ]; then
		debugfile=${file%.*}.dbg
		archive="${TMP_PATH}/${VERSION}_${name}.7z"
		archive_debug="${TMP_PATH}/${VERSION}_${name}_dbg.7z"
		${SEVENZIP} "${archive}" ${file}
		[ ! -f ${debugfile} ] || ${SEVENZIP} "${archive_debug}" ${debugfile}
	fi
}
#adds /data to 7z for AIInterface + SkirmishAI
#Usage: addata <archive> <root path to ai>
function adddata() {
	archive=$1
	rootpath=$2
	if [ -f ${archive} ]; then
		cd ${rootpath}
		${SEVENZIP} ${archive} VERSION data/
	fi
}

# Create one archive for base content and two archives for each exe/dll:
# one containing the exe/dll, and one containing debug symbols.
cd ${CONTENT_DIR}
# add LuaUI + fonts + misc files in root of cont to base archive
${SEVENZIP} ${BASE_ARCHIVE} . -x!base -x!freedesktop -xr!CMake* -xr!cmake* -xr!Makefile
# add already created 7z files (springcontent.sd7, maphelper.sd7, cursors.sdz, bitmaps.sdz) to base archive
cd ${BUILDDIR}
${SEVENZIP} ${BASE_ARCHIVE} ${BUILDDIR}/base

#create archive for needed dlls from mingwlibs
cd ${MINGWLIBS_DIR}
${SEVENZIP} ${MINGWLIBS_ARCHIVE} .

#create archive with ais + interfaces
cd ${BUILDDIR}
${SEVENZIP} ${AIS_ARCHIVE} AI -xr!CMakeFiles -xr!Makefile -xr!cmake_install.cmake -xr!*.dbg -xr!*.7z -xr!*.dev -xr!*.a -xr!sourceFiles.txt -xr!*.class -xr!*.java -xr!*.cpp -xr!*.h -xr!*.c -xr!VERSION

cd ${BUILDDIR}
for tostripfile in spring.exe spring-dedicated.exe spring-multithreaded.exe spring-headless.exe unitsync.dll; do
	zip ${tostripfile} ${tostripfile%.*}
done
for tostripfile in $(find AI/Skirmish -name SkirmishAI.dll); do
	name=$(basename $(dirname ${tostripfile}))
	zip ${tostripfile} $name
	adddata ${TMP_PATH}/${VERSION}_$name.7z $(dirname tostripfile)
done

for tostripfile in $(find AI/Interfaces -name AIInterface.dll); do
	name=$(basename $(dirname ${tostripfile}))
	zip ${tostripfile} $name
	adddata ${TMP_PATH}/${VERSION}_$name.7z $(dirname tostripfile)
done

cd $OLDPWD

# Rsync archives to a world-visible location.
if [ ${REMOTE_HOST} = localhost ] && [ -w ${REMOTE_BASE} ]; then
	${CMD} ${TMP_BASE}/ ${REMOTE_BASE}/
else
	${CMD} ${TMP_BASE}/ ${REMOTE_HOST}:${REMOTE_BASE}/
fi

# Clean up.
rm -rf ${TMP_BASE}/*

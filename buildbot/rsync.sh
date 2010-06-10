#!/bin/bash
. prepare.sh

REMOTE_HOST=localhost
REMOTE_BASE=/home/buildbot/www
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}
BASE_ARCHIVE="${TMP_PATH}/${VERSION}_base.7z"
CMD="rsync -avz --chmod=D+rx,F+r"

umask 022

# Usage: zip <file> <name>
function zip() {
	file=$1
	name=$2
	if [ -f ${file} ]; then
		debugfile=${file%.*}.dbg
		archive="${TMP_PATH}/${VERSION}_${name}.7z"
		archive_debug="${TMP_PATH}/${VERSION}_${name}_dbg.7z"
		7za u "${archive}" ${file}
		[ ! -f ${debugfile} ] || 7za u "${archive_debug}" ${debugfile}
	fi
}

# Create one archive for base content and two archives for each exe/dll:
# one containing the exe/dll, and one containing debug symbols.
cd ${BUILDDIR}
7za u "${BASE_ARCHIVE}" base
for tostripfile in spring.exe spring-dedicated.exe spring-gml.exe spring-hl.exe unitsync.dll; do
	zip ${tostripfile} ${tostripfile%.*}
done
for tostripfile in $(find AI/Skirmish -name SkirmishAI.dll); do
	zip ${tostripfile} $(basename $(dirname ${tostripfile}))
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

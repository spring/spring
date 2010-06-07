#!/bin/bash

# Quit on error.
set -e

cd $(dirname $0)/..

CONFIG=${1}
BRANCH=${2}
TMP_BASE=/tmp/spring
REMOTE_HOST=localhost
REMOTE_BASE=/home/buildbot/www
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}
REV=$(git describe --tags)
LOCAL_BASE=build-${CONFIG}
BASE_ARCHIVE=${TMP_PATH}/${REV}_base.7z
CMD="rsync -avz --chmod=D+rx,F+r"

umask 022

# Usage: zip <file> <name>
function zip() {
	file=$1
	name=$2
	if [ -f ${file} ]; then
		debugfile=${file%.*}.dbg
		archive=${TMP_PATH}/${REV}_${name}.7z
		archive_debug=${TMP_PATH}/${REV}_${name}_dbg.7z
		7z u ${archive} ${file}
		[ ! -f ${debugfile} ] || 7z u ${archive_debug} ${debugfile}
	fi
}

# Create one archive for base content and two archives for each exe/dll:
# one containing the exe/dll, and one containing debug symbols.
cd ${LOCAL_BASE}
7z u ${BASE_ARCHIVE} base
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

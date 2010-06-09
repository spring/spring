#!/bin/bash

# Quit on error.
set -e
cd $(dirname $0)/..

CONFIG=${1:-default}
BRANCH=${2:-master}
REV=$(git describe --tags)
FILE=rts/Game/GameVersion.cpp

if [ ${CONFIG} == default ]; then
	CONFIG=''
else
	CONFIG="[${CONFIG}]"
fi

if [ ${BRANCH} == master ]; then
	BRANCH=''
else
	BRANCH="{${BRANCH}}"
fi

VERSION="${CONFIG}${BRANCH}${REV}"
echo "setting version string '${VERSION}' in ${FILE}"
sed -i -r '/\bAdditional\s+=/ s/"[^"]*"/"'"${VERSION}"'"/; s/#define GV_ADD_SPACE ""/#define GV_ADD_SPACE " "/' ${FILE}

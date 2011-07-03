#!/bin/bash
set -e
. buildbot/slave/prepare.sh

FILE=rts/Game/GameVersion.cpp

echo "setting version string '${VERSION_}' in ${FILE}"

ESCAPED_VERSION=`echo "${VERSION_}" | sed 's@/@\\\\/@g'`
sed -i -r '/\badditional\s+=/ s/"[^"]*"/"'"${ESCAPED_VERSION}"'"/; s/#define GV_ADD_SPACE ""/#define GV_ADD_SPACE " "/' ${FILE}

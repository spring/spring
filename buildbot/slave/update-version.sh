#!/bin/bash
set -e
. buildbot/slave/prepare.sh

FILE=rts/Game/GameVersion.cpp

echo "setting version string '${VERSION}' in ${FILE}"
sed -i -r '/\badditional\s+=/ s/"[^"]*"/"'"${VERSION}"'"/; s/#define GV_ADD_SPACE ""/#define GV_ADD_SPACE " "/' ${FILE}

#!/bin/bash
. buildbot/slave/prepare.sh

FILE=rts/Game/GameVersion.cpp

echo "setting version string '${VERSION}' in ${FILE}"
sed -i -r '/\bAdditional\s+=/ s/"[^"]*"/"'"${VERSION}"'"/; s/#define GV_ADD_SPACE ""/#define GV_ADD_SPACE " "/' ${FILE}

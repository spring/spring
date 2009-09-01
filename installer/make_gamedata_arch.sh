#!/bin/sh
# Author: hoijui

# absolute or relative to spring source root
# default:
BUILD_DIR=game/base
# ... or use first argument to this script, if one was given
if [ $# -ge 1 ]
then
	BUILD_DIR=${1}
fi

# Sanity check.
if ! which 7z > /dev/null; then
	echo "Error: Could not find 7z."
	exit 1
fi
CMD_7Z="7z u -tzip -r -x!README.txt"

ORIG_DIR=$(pwd)

# Move to spring source root (eg containing dir 'installer')
cd $(dirname $0); cd ..

# Ensure directories exist (some VCSes do not support empty directories)
mkdir -p ${BUILD_DIR}/spring

# make the install dir absolute, if it is not yet
BUILD_DIR=$(cd $BUILD_DIR; pwd)

# Zip up the stuff.

cd installer/builddata/

echo Updating bitmaps.sdz
cd bitmaps/
${CMD_7Z} ${BUILD_DIR}/spring/bitmaps.sdz * > /dev/null
cd ..

echo Updating springcontent.sdz
cd springcontent/
${CMD_7Z} ${BUILD_DIR}/springcontent.sdz * > /dev/null
cd ..

echo Updating maphelper.sdz
cd maphelper/
${CMD_7Z} ${BUILD_DIR}/maphelper.sdz * > /dev/null
cd ..

echo Updating cursors.sdz
cd cursors/
${CMD_7Z} ${BUILD_DIR}/cursors.sdz * > /dev/null
cd ..

cd ../..
cd ${ORIG_DIR}


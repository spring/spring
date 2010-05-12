#!/bin/sh

# This will build the following, needed to run spring:
# * ${BUILD_DIR}/spring/bitmaps.sdz
# * ${BUILD_DIR}/springcontent.sdz
# * ${BUILD_DIR}/maphelper.sdz
# * ${BUILD_DIR}/cursors.sdz

# absolute or relative to spring source root
# default:
BUILD_DIR=build/base
# ... or use first argument to this script, if one was given
if [ $# -ge 1 ]
then
	BUILD_DIR=${1}
fi

if [ $# -ge 2 ]
then
	EXEC_7Z=${2}
else
	EXEC_7Z=7z
fi

# Sanity check.
if ! which ${EXEC_7Z} > /dev/null; then
	echo "Error: Could not find 7z."
	exit 1
fi
CMD_7Z="${EXEC_7Z} u -tzip -r"

ORIG_DIR=$(pwd)

# Move to spring source root
cd $(dirname $0); cd ../..

# Ensure directories exist (some VCSes do not support empty directories)
mkdir -p ${BUILD_DIR}/spring

# make the install dir absolute, if it is not yet
BUILD_DIR=$(cd ${BUILD_DIR}; pwd)

# Zip up the stuff.

cd cont/base/

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


#!/bin/sh
# This will build the following, needed to run spring:
# * ${BUILD_DIR}/spring/bitmaps.sdz
# * ${BUILD_DIR}/springcontent.sdz
# * ${BUILD_DIR}/maphelper.sdz
# * ${BUILD_DIR}/cursors.sdz

ORIG_DIR=$(pwd)

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
	echo "Error: Could not find 7z." >&2
	exit 1
fi
CMD_7Z="${EXEC_7Z} u -tzip -r"

# Move to spring source root
cd $(dirname $0); cd ../..

# Ensure directories exist (some VCSes do not support empty directories)
mkdir -p ${BUILD_DIR}/spring

# make the install dir absolute, if it is not yet
BUILD_DIR=$(cd ${BUILD_DIR}; pwd)
echo "Using build directory: ${BUILD_DIR}/ ..."

# Zip up the stuff.

cd cont/base/

echo "Creating spring/bitmaps.sdz"
cd bitmaps/
ARCHIVE_FILE=${BUILD_DIR}/spring/bitmaps.sdz
rm ${ARCHIVE_FILE}
${CMD_7Z} ${ARCHIVE_FILE} * > /dev/null
cd ..

echo "Creating springcontent.sdz"
cd springcontent/
ARCHIVE_FILE=${BUILD_DIR}/springcontent.sdz
rm ${ARCHIVE_FILE}
${CMD_7Z} ${ARCHIVE_FILE} * > /dev/null
cd ..

echo "Creating maphelper.sdz"
cd maphelper/
ARCHIVE_FILE=${BUILD_DIR}/maphelper.sdz
rm ${ARCHIVE_FILE}
${CMD_7Z} ${ARCHIVE_FILE} * > /dev/null
cd ..

echo "Creating cursors.sdz"
cd cursors/
ARCHIVE_FILE=${BUILD_DIR}/cursors.sdz
rm ${ARCHIVE_FILE}
${CMD_7Z} ${ARCHIVE_FILE} * > /dev/null
cd ..

cd ${ORIG_DIR}

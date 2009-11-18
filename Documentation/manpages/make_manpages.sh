#!/bin/sh
# Author: hoijui

# absolute or relative to spring source root
# default:
BUILD_DIR=game/manpages
# ... or use first argument to this script, if one was given
if [ $# -ge 1 ]
then
	BUILD_DIR=${1}
fi

if [ $# -ge 2 ]
then
	EXEC_ASCIIDOC=${2}
else
	EXEC_ASCIIDOC=asciidoc
fi

if [ $# -ge 3 ]
then
	EXEC_7Z=${3}
else
	EXEC_7Z=7z
fi

# Sanity check.
if ! which ${EXEC_ASCIIDOC} > /dev/null; then
	echo "Error: Could not find asciidoc."
	exit 1
fi
if ! which ${EXEC_7Z} > /dev/null; then
	echo "Error: Could not find 7z."
	exit 1
fi

SRC_DIR=$(pwd)

# Move to spring source root (eg containing dir 'installer')
cd $(dirname $0); cd ..

# Ensure directories exist (some VCSes do not support empty directories)
mkdir -p ${BUILD_DIR}

# make the install dir absolute, if it is not yet
BUILD_DIR=$(cd $BUILD_DIR; pwd)

cd ${BUILD_DIR}



# copy sources to build dir
cp ${SRC_DIR}/*.6.txt ${BUILD_DIR}

for manFile_src in ${BUILD_DIR}/*.6.txt
do
	# strip off the extension
	manFile=${manFile_src%.*}
	manFile_raw=${manFile}.backend
	manFile_cmp=${manFile}.gz

	# compile the source
	${EXEC_ASCIIDOC} --doctype=manpage --backend=docbook --out-file=${manFile_raw} - < ${manFile_src} > /dev/null
	# compress the result
	${EXEC_7Z} a -tgzip "${manFile_cmp}" "${manFile_raw}" > /dev/null
done

# delete sources from build dir
rm ${BUILD_DIR}/*.6.txt

cd ${SRC_DIR}


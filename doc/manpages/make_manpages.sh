#!/bin/sh

ORIG_DIR=$(pwd)

# absolute or relative to spring source root
# default:
BUILD_DIR=build/manpages
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
	EXEC_XSLTPROC=${3}
else
	EXEC_XSLTPROC=xsltproc
fi

if [ $# -ge 4 ]
then
	XSL_DOCBOOK=${4}
else
	XSL_DOCBOOK=/usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl
fi

if [ $# -ge 5 ]
then
	EXEC_7Z=${5}
else
	EXEC_7Z=7z
fi

# Sanity check.
if ! which ${EXEC_ASCIIDOC} > /dev/null; then
	echo "Error: Could not find asciidoc." >&2
	exit 1
fi
if ! which ${EXEC_XSLTPROC} > /dev/null; then
	echo "Error: Could not find xsltproc." >&2
	exit 1
fi
if [ ! -f ${XSL_DOCBOOK} ]; then
	echo "Error: Could not find docbook.xsl." >&2
	exit 1
fi
if ! which ${EXEC_7Z} > /dev/null; then
	echo "Error: Could not find 7z." >&2
	exit 1
fi

SRC_DIR=$(cd $(dirname $0); pwd)

# Move to spring source root (eg containing dir 'installer')
cd ${SRC_DIR}; cd ../..

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
	manFile_xml=${manFile}.xml
	manFile_man=${manFile}
	manFile_cmp=${manFile}.gz

	# compile
	${EXEC_ASCIIDOC} --doctype=manpage --backend=docbook --out-file="${manFile_xml}" - < "${manFile_src}" > /dev/null
	# format
	${EXEC_XSLTPROC} --output "${manFile_man}" "${XSL_DOCBOOK}" "${manFile_xml}" > /dev/null
	# archive
	${EXEC_7Z} a -tgzip "${manFile_cmp}" "${manFile_man}" > /dev/null
done

# delete sources from build dir
rm ${BUILD_DIR}/*.6.txt

cd ${ORIG_DIR}

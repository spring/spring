#!/bin/bash

set -e

if [ $# -lt 1 ]; then
	echo "Usage: $0 <spring_VERSION.exe> <outputpath>"
	exit
fi

# check req.
`which wine &> /dev/null` || (echo "Error: You need wine installed!"; exit)
`which winepath &> /dev/null` || (echo "Error: You need winepath installed!"; exit)
`which 7z &> /dev/null` || (echo "Error: You need 7z installed!"; exit)


# prepare
INSTALLER=$1
OUTPUTPATH=$2

if [ -n ${OUTPUTPATH} ]; then
	if [ ! -d ${OUTPUTPATH} ]; then
		echo "${OUTPUTPATH} doesn't exist!"
		exit 1
	fi
	OUTPUTPATH="${OUTPUTPATH}/"
fi

VERSION=`basename ${INSTALLER} .exe`
VERSION=${VERSION:7}
echo ${VERSION} detected
TEMPDIR=`mktemp -d`

echo Temporary directory: ${TEMPDIR}

# create a tempdir
PATHNAME=spring-${VERSION}
INSTPATH=${TEMPDIR}/${PATHNAME}
mkdir -p "${INSTPATH}"

# detect wine `windows` peth of the tempdir
WINEINSTPATH=`winepath -w "${INSTPATH}"`
if [ "${WINEINSTPATH:0:4}" = "\\\\?\\" ]; then
	echo "Error: Couldn't translate tempdir path"
	exit
fi

# make the path passable via linux bash \ -> \\
WINEINSTPATH="${WINEINSTPATH//\\/\\\\}"

# install in tempdir via wine
INSTCOMMAND="wine ${INSTALLER} /S /PORTABLE /NOREGISTRY /NODESKTOPLINK /NOSTARTMENU /D=$WINEINSTPATH"
echo $INSTCOMMAND

if ! sh -c "$INSTCOMMAND" ; then
	echo "Error: Installation failed"
	exit
fi

# compress
7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=64m -ms=on ${OUTPUTPATH}spring_${VERSION}_portable.7z ${INSTPATH}

rm -rf ${TEMPDIR}


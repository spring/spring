#!/bin/bash

if [ $# != 1 ]; then
	echo "Usage: $0 <spring_VERSION.exe>"
	exit
fi

# check req.
`which wine &> /dev/null` || (echo "Error: You need Wine installed!"; exit)
`which 7z &> /dev/null` || (echo "Error: You need 7z installed!"; exit)

# prepare
INSTALLER=$1
VERSION=`basename ${INSTALLER} .exe`
VERSION=${VERSION:7}
echo ${VERSION} detected
TEMPDIR=`mktemp -d`

# create a tempdir
PATHNAME=spring-${VERSION}
INSTPATH=${TEMPDIR}/${PATHNAME}
mkdir -p "${INSTPATH}"

# detect wine `windows` peth of the tempdir
WINEINSTPATH=`winepath -w "${INSTPATH}"`
if [ "${WINEINSTPATH:0:4}" = "\\\\?\\" ]; then
	echo "Error: Couldn't translate tempdir path"
	echo we live secure, do this yourself:
	echo rm -f ${TEMPDIR}
	exit
fi

# make the path passable via linux bash \ -> \\
WINEINSTPATH="${WINEINSTPATH//\\/\\\\}"

# install in tempdir via wine
INSTCOMMAND="wine ${INSTALLER} /S /PORTABLE /NOREGISTRY /NODESKTOPLINK /NOSTARTMENU /D=$WINEINSTPATH"
echo $INSTCOMMAND

if ! sh -c "$INSTCOMMAND" ; then
	echo "Error: Installation failed"
	echo we live secure, do this yourself:
	echo rm -f ${TEMPDIR}
	exit
fi

# compress
7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on spring_${VERSION}_portable.7z ${INSTPATH}

# finished
echo we live secure, do this yourself:
echo rm -f ${TEMPDIR}


#!/bin/bash

if [ $# != 1 ]; then
	echo "Usage: $0 <spring_VERSION.exe>"
	exit
fi

INSTALLER=$1
VERSION=`basename ${INSTALLER} .exe`
VERSION=${VERSION:7}
echo ${VERSION} detected
TEMPDIR=`mktemp -d`

PATHNAME=spring-${VERSION}
INSTPATH=${TEMPDIR}/${PATHNAME}
mkdir -p ${INSTPATH}
#convert to winepath, Z:\ is /

WINEINSTPATH="Z:${INSTPATH//\//\\\\}"

INSTCOMMAND="wine ${INSTALLER} /S /PORTABLE /NOREGISTRY /NODESKTOPLINK /NOSTARTMENU /D=$WINEINSTPATH"
echo $INSTCOMMAND

if ! sh -c "$INSTCOMMAND" ; then
	echo "Installation failed"
	exit
fi

7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on spring_${VERSION}_portable.7z ${INSTPATH}

echo we live secure, do this yourself:
echo rm -f ${TEMPDIR}


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

echo wine ${INSTALLER} /S /D=$WINEINSTPATH

if ! sh -c "wine ${INSTALLER} /S /D=$WINEINSTPATH" ; then
	echo "Installation failed"
	exit
fi

touch ${INSTPATH}/springsettings.cfg ${INSTPATH}/springlobby.conf

7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on spring_${VERSION}_portable.7z ${INSTPATH}

echo we live secure, do this yourself:
echo rm -f ${TEMPDIR}


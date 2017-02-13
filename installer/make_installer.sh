#! /bin/bash

# Quit on error.
set -e

if [ ! -e installer ]; then
	echo "Error: This script needs to be run from the root directory of the archive"
	exit 1
fi
WGET="wget -N --cache=off"

TAG=$(git describe --tags|tr -d '\n')

if [ "$TAG" = "" ]; then
	echo "Error running git describe"
	exit 1
fi
NSISDEFINES="-DVERSION_TAG="$TAG""

# Evaluate the engines version
if ! git describe --candidate=0 --tags 2>/dev/null; then
	NSISDEFINES="$NSISDEFINES -DTEST_BUILD"
	echo "Creating test installer for revision $TAG"
fi

mkdir -p installer/downloads
cd installer/downloads

if ! [ -s vcredist_x86.exe ]; then
	$WGET http://download.microsoft.com/download/e/1/c/e1c773de-73ba-494a-a5ba-f24906ecf088/vcredist_x86.exe
fi

if [ ! -s spring_testing_minimal-portable.7z ]; then
	echo "Warning: spring_testing_minimal-portable.7z didn't exist, downloading..." >&2
	$WGET https://springrts.com/dl/buildbot/default/master/spring_testing_minimal-portable.7z
fi

cd ../..

#create uninstall.nsh
installer/make_uninstall_nsh.py installer/downloads/spring_testing_minimal-portable.7z >installer/downloads/uninstall.nsh


makensis -V3 $NSISDEFINES $@ -DNSI_UNINSTALL_FILES=downloads/uninstall.nsh \
-DMIN_PORTABLE_ARCHIVE=downloads/spring_testing_minimal-portable.7z \
-DVCREDIST=downloads/vcredist_x86.exe \
 installer/spring.nsi


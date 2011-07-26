#! /bin/bash

# Quit on error.
set -e

if [ ! -e installer ]; then
	echo "Error: This script needs to be run from the root directory of the archive"
	exit 1
fi
WGET="wget -N"

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

$WGET http://springrts.com/dl/rapid-spring-latest-win32.7z
$WGET http://springrts.com/dl/TASServer.jar
$WGET http://zero-k.info/lobby/setup.exe
$WGET http://zero-k.info/lobby/setup_icon.ico
$WGET http://www.springlobby.info/windows/latest.zip

cd ..
rm -rf Springlobby
mkdir -p Springlobby
cd Springlobby
unzip ../downloads/latest.zip -d SLArchive

mkdir SettingsDlls
mv SLArchive/*.dll SettingsDlls
mv SLArchive/springsettings.exe SettingsDlls


### This version of OpenAL breaks Spring ...
# not used anymore since May 2010
rm -f SettingsDlls/OpenAL32.dll
cd ../..
makensis -V3 $NSISDEFINES $@ -DNSI_UNINSTALL_FILES=sections/uninstall.nsh -DRAPID_ARCHIVE=downloads/rapid-spring-latest-win32.7z installer/spring.nsi


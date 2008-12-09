#!/bin/sh

# Author: Tobi Vollebregt
# Initial version copied from make_test_installer.bat

# Quit on error.
set -e

# Find correct working directory.
# (Compatible with SConstruct, which is in trunk root)

while [ ! -d installer ]; do
        if [ "$PWD" = "/" ]; then
                echo "Error: Could not find installer directory."
                echo "Make sure to run this script from a directory below your checkout directory."
                exit 1
        fi
        cd ..
done


# Acuire AI Interface and Skirmish AI Version information

AI_VERSION_INFO="-DINSTALL_AIS"

cd AI/Interfaces
for interfaceDir in `ls`; do
	if [ -f ${interfaceDir}/VERSION ]; then
		interfaceVersion=`cat ${interfaceDir}/VERSION`
		AI_VERSION_INFO="$AI_VERSION_INFO -DAI_INT_VERS_${interfaceDir}=${interfaceVersion}"
	fi
done
cd ../..

cd AI/Skirmish
for skirmishDir in `ls`; do
	if [ -f ${skirmishDir}/VERSION ]; then
		skirmishVersion=`cat ${skirmishDir}/VERSION`
		AI_VERSION_INFO="$AI_VERSION_INFO -DSKIR_AI_VERS_${skirmishDir}=${skirmishVersion}"
	fi
done
cd ../..

echo AI_VERSION_INFO: $AI_VERSION_INFO

# Got a revision argument?

if [ -z "$1" ]; then
	REVISION=`svnversion | grep -o -E '[0-9]+'`
else
	REVISION=$1
fi
echo "Creating installers for revision $REVISION"

# Figure out whether this is a test build or a release build,
# and build the correct full and updating installers.
# The grep regex matches the ones used in buildbot.

if grep -o -E 'const char\* const.*\+' rts/Game/GameVersion.cpp >/dev/null; then
	echo "Creating test installer"
	makensis -V3 $AI_VERSION_INFO -DTEST_BUILD -DREVISION=$REVISION installer/spring.nsi

	#echo "Creating updating test installer"
	#makensis -V3 -DSP_UPDATE -DTEST_BUILD -DREVISION=$REVISION installer/spring.nsi
else
	echo "Creating installer"
	makensis -V3 $AI_VERSION_INFO installer/spring.nsi

	#echo "Creating updating installer"
	#makensis -V3 -DSP_UPDATE installer/spring.nsi
fi

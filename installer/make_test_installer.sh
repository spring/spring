#!/bin/sh

# Author: Tobi Vollebregt
# Initial version copied from make_test_installer.bat

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

# TODO: code to d/l & extract extra files for installer

# Follows a slightly modified copy of make_test_installer.bat:

echo This will create the various installers for a TA Spring release
echo .
echo Remember to generate all needed path information for the maps to be included
echo .
echo Also remember to update the version string in GameVersion.h so that
echo the correct .pdb can be identified!
echo .

if [ "$#" != "0" ]; then
	echo Creating nightly installer of revision $1
	defines="-DNIGHTLY_BUILD -DREVISION=$1"
else
	echo Creating test installer
	defines="-DTEST_BUILD"
fi

echo .
makensis -V3 -DNO_TOTALA -DMINGW $defines installer/taspring.nsi || exit 1

echo .
echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..

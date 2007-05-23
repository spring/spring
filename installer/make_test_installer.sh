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

# Got a revision argument?

if [ -z "$1" ]; then
	REVISION=`svnversion | grep -o -E '[0-9]+'`
else
	REVISION=$1
fi

# A slightly modified copy of make_test_installer.bat follows:

echo This will create the various installers for a TA Spring release
echo .
echo Remember to generate all needed path information for the maps to be included
echo .
echo Also remember to update the version string in GameVersion.h so that
echo the correct .pdb can be identified!
echo .

echo Creating test installer
makensis -V3 -DTEST_BUILD -DREVISION=$REVISION installer/taspring.nsi || exit 1

echo Creating updating test installer
makensis -V3 -DSP_UPDATE -DTEST_BUILD -DREVISION=$REVISION installer/taspring.nsi || exit 1

echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..

#!/bin/sh

# Author: Tobi Vollebregt
# Initial version copied from make_test_installer.bat

# Find correct working directory.
# (Compatible with SConstruct, which is in trunk root)

while [ ! -d installer ]; do
        if [ "$PWD" == "/" ]; then
                echo "Error: Could not find installer directory."
                echo "Make sure to run this script from a directory below your checkout directory."
                exit 1
        fi
        cd ..
done

# Follows a slightly modified copy of make_test_installer.bat:

echo This will create the various installers for a TA Spring release
echo .
echo Remember to generate all needed path information for the maps to be included
echo Press any key to continue..
echo .
echo Also remember to update the version string in GameVersion.h so that
echo the correct .pdb can be identified!
echo .

echo Creating test installer
makensis -V3 -DTEST_BUILD -DMINGW installer/taspring.nsi

echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..

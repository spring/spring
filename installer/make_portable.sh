#!/bin/sh
# Author: Karl-Robert Ernst

# Quit on error.
set -e

# Sanity check.
if [ ! -x /usr/bin/git ]; then
	echo "Error: Couldn't find /usr/bin/git"
	exit 1
fi

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

sh -c 'installer/springlobby_download.sh'

set +e # turn of quit on error
git describe --candidates 0 --tags &> /dev/null
if [ $? -ne "0" ]; then
	RELEASE_SOURCE=false
	echo "Making test-packages"
else
	RELEASE_SOURCE=true
	echo "Making release-packages"
fi
set -e # turn it on again

if [ $RELEASE_SOURCE ]; then
	version_string=`git describe --tags`
	branch=${version_string}
else
	version_string=`git describe --tags | sed s/\-[^\-]*$//`
	branch="master"
fi
echo "Using $branch as source"

# temp fix for shared libgcc so a proper one can be committed when somebody
# notices :P
CheckFiles=("game/spring.exe" "installer/downloads/springlobby.exe" "installer/downloads/springsettings.exe" "game/unitsync.dll" "installer/downloads/libgcc_s_dw2-1.dll")
for filename in ${CheckFiles[@]}
do
	if [ ! -f $filename ]; then
		echo "Error: Couldn't find $filename"
		exit 1
	fi
done

archive_filename="spring_${version_string}_portable_win32.zip"
dir_in_archive="spring_${version_string}"

rm -rf $dir_in_archive
mkdir -p $dir_in_archive
cp -r game/* $dir_in_archive
cp installer/downloads/*.exe installer/downloads/TASServer.jar $dir_in_archive
cp -r installer/Springlobby/SettingsDlls/* installer/Springlobby/SLArchive/* $dir_in_archive
touch $dir_in_archive/springlobby.conf # make SL go into portable mode
touch $dir_in_archive/springsettings.cfg # make spring go into portable mode

/usr/bin/zip -q -r -u -9 "$archive_filename" $dir_in_archive

rm -rf $dir_in_archive

#!/bin/sh
# Author: Tobi Vollebregt
#
# Hook called just after svn checkout/update in buildbot crosscompile build.
# Gets passed one argument: a revision number.
#

if [ "$#" != "1" ]; then
	echo "Usage: $0 <revision>"
	exit 1
fi

# 7z installed?
if [ -z "`which 7z`" ]; then
	echo 7z not found
	exit 1
fi

# zip installed?
if [ -z "`which zip`" ]; then
	echo zip not found
	exit 1
fi

# Extract files
root="/var/lib/buildbot/spring_slave"

# Libs/includes needed for crosscompilation
7z x -y "$root/mingwlibs.exe" || exit 1
find mingwlibs -type f -exec chmod 644 '{}' \;
find mingwlibs -type d -exec chmod 755 '{}' \;
# Files needed for installer building
#tar xzfv "$root/extracontent.tar.gz" || exit 1

# Update version
if [ "$#" != "0" ]; then
	echo "updating GameVersion.h to revision $1"
	sed -i "/VERSION_STRING/s,\"[^\"]*\",\"r$1\",g" rts/Game/GameVersion.h || exit 1
fi

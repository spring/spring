#!/bin/sh
#
# Author: Tobi Vollebregt
#
# Hook called just after svn checkout/update in buildbot crosscompile build.
# May get one argument: a revision number. If so, it updates GameVersion.h
# to this revision number.
#

set -e

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
7z x -y "$root/mingwlibs.exe"
find mingwlibs -type f -exec chmod 644 '{}' \;
find mingwlibs -type d -exec chmod 755 '{}' \;
# Files needed for installer building
#tar xzfv "$root/extracontent.tar.gz"

# Update version
if [ "$#" != "0" ] && [ -n "$1" ]; then
	echo "updating GameVersion.h to revision $1"
	sed -i "/VERSION_STRING/s,\"[^\"]*\",\"r$1\",g" rts/Game/GameVersion.h || exit 1
fi

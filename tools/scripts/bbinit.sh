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

quit=0

# 7z installed?
if [ "x$(which 7z)" = "x" ]; then
	echo 7z not found
	quit=1
fi

# zip installed?
if [ "x$(which zip)" = "x" ]; then
	echo zip not found
	quit=1
fi

if [ "x$quit" != "x0" ]; then
	exit 1
fi

# Extract files
root="/home/tvo/Buildbot/spring_slave"
# Libs/includes needed for crosscompilation
7z x -y "$root/mingwlibs.exe" | grep 'Extracting  ' | sed 's/Extracting  //g' | tee files
# Files needed for installer building
tar xzfv "$root/extracontent.tar.gz" | tee -a files

# touch files so the cleanup scripts dont kill them
# (note how it can still happen that cleanup scripts kill the files just between
# extracting and touching them, no clue how to solve that....)
cat files | xargs touch

# Update version
if [ "$#" != "0" ]; then
	echo "updating GameVersion.h to revision $1"
	sed -i "/VERSION_STRING/s,\"[^\"]*\",\"r$1\",g" rts/Game/GameVersion.h || exit 1
fi

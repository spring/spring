#!/bin/sh
# Author: Tobi Vollebregt

# absolute or relative to spring source root
# default:
BUILD_DIR=game/base
# ... or use first argument to this script, if one was given
if [ $# -ge 1 ]
then
	BUILD_DIR=${1}
fi

# Sanity check.
if ! which zip >/dev/null; then
	echo "Error: Could not find zip."
	exit 1
fi

# Move to spring source root (eg containing dir 'installer')
cd $(dirname $0); cd ..

# Ensure directories exist (some VCSes do not support empty directories)
mkdir -p ${BUILD_DIR}/spring

# make the install dir absolute, if it is not yet
BUILD_DIR=$(cd $BUILD_DIR; pwd)

# Zip up the stuff.

cd installer/builddata/

echo Updating bitmaps.sdz
cd bitmaps/
zip -qu ${BUILD_DIR}/spring/bitmaps.sdz modinfo.lua
zip -qu ${BUILD_DIR}/spring/bitmaps.sdz bitmaps/*
zip -qu ${BUILD_DIR}/spring/bitmaps.sdz bitmaps/*/*
cd ..

echo Updating springcontent.sdz
cd springcontent/
zip -qu ${BUILD_DIR}/springcontent.sdz modinfo.lua
zip -qu ${BUILD_DIR}/springcontent.sdz EngineOptions.lua
zip -qu ${BUILD_DIR}/springcontent.sdz gamedata/*
zip -qu ${BUILD_DIR}/springcontent.sdz gamedata/*/*
zip -qu ${BUILD_DIR}/springcontent.sdz bitmaps/*
zip -qu ${BUILD_DIR}/springcontent.sdz bitmaps/*/*
zip -qu ${BUILD_DIR}/springcontent.sdz anims/*
zip -qu ${BUILD_DIR}/springcontent.sdz shaders/*
zip -qu ${BUILD_DIR}/springcontent.sdz LuaGadgets/*
cd ..

echo Updating maphelper.sdz
cd maphelper/
zip -qu ${BUILD_DIR}/maphelper.sdz modinfo.lua
zip -qu ${BUILD_DIR}/maphelper.sdz MapOptions.lua
zip -qu ${BUILD_DIR}/maphelper.sdz maphelper/*
cd ..

echo Updating cursors.sdz
cd cursors/
zip -qu ${BUILD_DIR}/cursors.sdz modinfo.lua
zip -qu ${BUILD_DIR}/cursors.sdz anims/*
cd ..

cd ../..

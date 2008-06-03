#!/bin/sh
# Author: Tobi Vollebregt

# Sanity check.
if ! which zip >/dev/null; then
	echo "Error: Could not find zip."
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

# Zip up the stuff.

cd installer/builddata/

echo Updating bitmaps.sdz
cd bitmaps/
zip -qu ../../../game/base/spring/bitmaps.sdz modinfo.tdf
zip -qu ../../../game/base/spring/bitmaps.sdz bitmaps/*
zip -qu ../../../game/base/spring/bitmaps.sdz bitmaps/*/*
cd ..

echo Updating springcontent.sdz
cd springcontent/
zip -qu ../../../game/base/springcontent.sdz modinfo.tdf
zip -qu ../../../game/base/springcontent.sdz gamedata/*
zip -qu ../../../game/base/springcontent.sdz gamedata/*/*
zip -qu ../../../game/base/springcontent.sdz bitmaps/*
zip -qu ../../../game/base/springcontent.sdz bitmaps/*/*
zip -qu ../../../game/base/springcontent.sdz anims/*
zip -qu ../../../game/base/springcontent.sdz shaders/*
zip -qu ../../../game/base/springcontent.sdz LuaGadgets/*
cd ..

echo Updating maphelper.sdz
cd maphelper/
zip -qu ../../../game/base/maphelper.sdz modinfo.tdf
zip -qu ../../../game/base/maphelper.sdz MapOptions.lua
zip -qu ../../../game/base/maphelper.sdz maphelper/*
cd ..

cd ../..

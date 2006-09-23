#!/bin/sh
# Author: Tobi Vollebregt

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

# Zip up the stuff.

echo Updating bitmaps.sdz
cd installer/builddata/bitmaps
zip -qu ../../../game/base/spring/bitmaps.sdz modinfo.tdf bitmaps/* bitmaps/*/* Anims/*
echo Updating springcontent.sdz
cd ../springcontent
zip -qu ../../../game/base/springcontent.sdz modinfo.tdf gamedata/* gamedata/*/*
cd ../../..

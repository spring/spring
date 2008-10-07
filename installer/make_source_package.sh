#!/bin/sh
# Author: Tobi Vollebregt

# Quit on error.
set -e

# Sanity check.
if [ ! -x /usr/bin/svn ]; then
	echo "Error: Couldn't find /usr/bin/svn"
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

# This regex matches regexes in buildbot etc.
version=`grep -o -E '0\.[0-9]{2,2}[b.][0-9]\+?(svn[0-9]+)?' rts/Game/GameVersion.cpp`

# Each one of these that is set is build when running this script.
# .tar.bz2 and .tar.gz are built with linux (LF) line endings.
# .zip and .7z are built with windows (CRLF) line endings.
dir="spring_$version"
tbz="spring_${version}_src.tar.bz2"
#tgz="spring_${version}_src.tar.gz"
zip="spring_${version}_src.zip"
#seven_zip="spring_${version}_src.7z"

# This is the list of files/directories that go in the source package.
# (directories are included recursively)
include=" \
 $dir/AI/Group/ \
 $dir/AI/Global/AAI/ \
 $dir/AI/Global/KAIK-0.13/ \
 $dir/AI/Global/TestGlobalAI/ \
 $dir/Documentation/ \
 $dir/Doxyfile \
 $dir/game/ \
 $dir/installer/ \
 $dir/LICENSE.html \
 $dir/README.* \
 $dir/rts/ \
 $dir/SConstruct \
 $dir/tools/SelectionEditor/ \
 $dir/CMakeLists.txt \
 $dir/AI/CMakeLists.txt \
 $dir/tools/unitsync/ \
 $dir/tools/DedicatedServer/"

# On linux, win32 executables are useless.
# TASClient is windows only.
linux_exclude="$dir/installer/pkzip.exe"
linux_include=""
windows_exclude=""
windows_include=""

# Linux line endings, .tar.{bz2,gz} package.
echo 'Exporting checkout dir with LF line endings'
mkdir lf || exit 1
cd lf
/usr/bin/svn export .. "$dir" --native-eol LF
[ -n "$linux_exclude" ] && rm -rf $linux_exclude
[ -n "$tbz" ] && echo "Creating .tar.bz2 archive ($tbz)" && \
	tar cfj "../$tbz" $include $linux_include
[ -n "$tgz" ] && echo "Creating .tar.gz archive ($tgz)" && \
	tar cfz "../$tgz" $include $linux_include
cd ..
echo 'Cleaning'
rm -rf lf

# Windows line endings, .zip/.7z package
echo 'Exporting checkout dir with CRLF line endings'
mkdir crlf || exit 1
cd crlf
/usr/bin/svn export .. "$dir" --native-eol CRLF
[ -n "$windows_exclude" ] && rm -rf $windows_exclude
[ -n "$zip" ] && [ -x /usr/bin/zip ] && echo "Creating .zip archive ($zip)" && \
	/usr/bin/zip -q -r -u -9 "../$zip" $include $windows_include
[ -n "$seven_zip" ] && [ -x /usr/bin/7z ] && echo "Creating .7z archive ($seven_zip)" && \
	/usr/bin/7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on "../$seven_zip" $include >/dev/null
cd ..
echo 'Cleaning'
rm -rf crlf

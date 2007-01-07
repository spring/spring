#!/bin/sh
# Author: Tobi Vollebregt

# Sanity checks.
if [ -z "$1" ]; then
	echo "Usage: $0 <version>"
	echo "Example: $0 0.72b1"
	exit 1
fi

if [ ! -x /usr/bin/svn ]; then
	echo "ERROR: Couldn't find /usr/bin/svn"
	exit 1
fi

version="$1"
svnbase="https://taspring.clan-sy.com/svn/spring"

if [ "$version" = "trunk" ]; then
	svnurl="$svnbase/trunk"
	#version="trunk-r`svn info $svnbase/trunk | grep Revision | sed 's/^Revision: //g'`"
	# Find checkout directory.
	svndir="."
	while [ ! -d "$svndir/installer" ]; do
        	if [ "`( cd "$svndir" && echo "$PWD" )`" = "/" ]; then
                	echo "Error: Could not find installer directory."
	                echo "Make sure to run this script from a directory below your checkout directory."
        	        exit 1
	        fi
		if [ "$svndir" = "." ]; then
			svndir=".."
		else
	        	svndir="$svndir/.."
		fi
	done
else
	svnurl="$svnbase/tags/spring_$version"
	svndir="spring_$version"
fi

dir="spring_$version"
tbz="spring_${version}_src.tar.bz2"
tgz="spring_${version}_src.tar.gz"
zip="spring_${version}_src.zip"
seven_zip="spring_${version}_src.7z"

# This is the list of files/directories that go in the source package.
# (directories are included recursively)
include=" \
 $dir/AI/Group/ \
 $dir/AI/Global/AAI/ \
 $dir/AI/Global/TestGlobalAI/ \
 $dir/Documentation/ \
 $dir/Doxyfile \
 $dir/game/ \
 $dir/installer/ \
 $dir/LICENSE.html \
 $dir/lua/ \
 $dir/README.* \
 $dir/rts/ \
 $dir/SConstruct \
 $dir/tools/unitsync/ \
 $dir/TASClient \
 $dir/TASServer \
 $dir/UnityLobby"

# On linux, win32 executables are useless.
linux_exclude="$dir/installer/pkzip.exe $dir/TASClient/brcc32.exe"
windows_exclude=""

# Update/create SVN checkout.
if [ -d "$svndir" ]; then
	olddir="$PWD"
	cd "$svndir"
	echo '-> UPDATING SVN CHECKOUT'
	/usr/bin/svn update || exit 1
	cd "$olddir"
else
	echo '-> CREATING SVN CHECKOUT'
	/usr/bin/svn checkout "$svnurl" "$svndir" || exit 1
fi

# Linux line endings, .tar.{bz2,gz} package.
echo '-> EXPORTING CHECKOUT DIR WITH LF LINE ENDINGS'
mkdir lf || exit 1
cd lf
/usr/bin/svn export "../$svndir" "$dir" --native-eol LF || exit 1
[ -n "$linux_exclude" ] && rm -rf $linux_exclude
echo '-> CREATING .TAR.BZ2 ARCHIVE'
tar cfj "../$tbz" $include
echo '-> CREATING .TAR.GZ ARCHIVE'
tar cfz "../$tgz" $include
cd ..
echo '-> CLEANING'
rm -rf lf

# Windows line endings, .zip/.7z package
echo '-> EXPORTING CHECKOUT DIR WITH CRLF LINE ENDINGS'
mkdir crlf || exit 1
cd crlf
/usr/bin/svn export "../$svndir" "$dir" --native-eol CRLF || exit 1
[ -n "$windows_exclude" ] && rm -rf $windows_exclude
[ -x /usr/bin/zip ] && echo '-> CREATING .ZIP ARCHIVE' && \
	/usr/bin/zip -q -r -u -9 "../$zip" $include
[ -x /usr/bin/7z ] && echo '-> CREATING .7Z ARCHIVE' && \
	/usr/bin/7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on "../$seven_zip" $include >/dev/null
cd ..
echo '-> CLEANING'
rm -rf crlf

#!/bin/sh
# Author: Tobi Vollebregt

if [ "x$1" == "x" ]; then
	echo "Usage: $0 <version>"
	echo "Example: $0 0.72b1"
	exit 1
fi

if [ ! -x /usr/bin/svn ]; then
	echo "ERROR: Couldn't find /usr/bin/svn"
	exit 1
fi

version=$1
svnbase=https://taspring.clan-sy.com/svn/spring
svnurl=$svnbase/tags/taspring_$version
dir=taspring_$version
tar=taspring_${version}_src.tar.bz2
zip=taspring_${version}_src.zip
seven_zip=taspring_${version}_src.7z

if [ -d $dir ]; then
	cd $dir
	/usr/bin/svn update || exit 1
	cd ..
else
	/usr/bin/svn checkout $svnurl $dir || exit 1
fi

# Linux line endings, .tar.bz2 package.
mkdir lf
cd lf
/usr/bin/svn export ../$dir $dir --native-eol LF || exit 1
tar cfj ../$tar $dir
cd ..
rm -rf lf

# Windows line endings, .zip/.7z package
mkdir crlf
cd crlf
/usr/bin/svn export ../$dir $dir --native-eol CRLF || exit 1
[ -x /usr/bin/zip ] && /usr/bin/zip -r -9 ../$zip $dir
[ -x /usr/bin/7z ] && /usr/bin/7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ../$seven_zip $dir
cd ..
rm -rf crlf

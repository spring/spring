#!/bin/bash
PACKAGE=`dpkg-parsechangelog -c1 | sed -rn "s/^Source: (.+)/\1/p"`
VERSION=`dpkg-parsechangelog -c1 | sed -rn "s/^Version: (.+)-.+$/\1/p"`
TMPDIR=`mktemp -d`
SOURCEDIR="$PACKAGE-$VERSION"
SOURCETARGZ="${PACKAGE}_$VERSION.orig.tar.gz"
TARGET=$PWD

svn export . "$TMPDIR/$SOURCEDIR"
cd "$TMPDIR"
tar -zcvf "$SOURCETARGZ" "$SOURCEDIR"
cd "$SOURCEDIR"
debuild -S -sa
cd "$TMPDIR"
rm -rf "$SOURCEDIR"
cd $TARGET
rm -rf release
mv $TMPDIR release

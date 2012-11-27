#!/bin/sh

set -e

if [ $# != 3 ]; then
	echo Usage: $0 config branch /path/to/mingwlibs
	exit 1
fi

. buildbot/slave/prepare.sh

if [ ! -d "$1" ]; then
	mkdir -p "$1"
	cd "$1"
	git clone git://github.com/spring/mingwlibs.git
fi

cd "$1"

GITOUTPUT=$(git fetch 2>&1 |cat)
echo $GITOUTPUT
if [ -n "$GITOUTPUT" ];
then
	echo "removing builddir $BUILDDIR (mingwlibs updated)"
	rm -rf "$BUILDDIR"
	git clean -f -d
	git reset --hard origin/master
fi


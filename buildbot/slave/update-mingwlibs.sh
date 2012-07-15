#!/bin/sh

set -e

if [ $# != 3 ]; then
	echo Usage: $0 config branch /path/to/mingwlibs
	exit 1
fi

. buildbot/slave/prepare.sh

cd $1
if [ -n "$(git fetch)" ]; then
	# remote contains new commits, delete builddir
	rm -rf "$BUILDDIR"
	git clean -f -d
	git reset --hard origin/master
fi


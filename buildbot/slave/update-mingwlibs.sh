#!/bin/sh

set -e
if [ $# != 1 ]; then
	echo Usage: $0 /path/to/mingwlibs
	exit 1
fi

cd $1
git fetch
git clean -f -d
git reset --hard origin/master


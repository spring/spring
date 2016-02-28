#!/bin/sh

set -e


. buildbot/slave/prepare.sh

if [ -z $MINGWLIBS_PATH ]; then
	echo 'MINGWLIBS_PATH not set'
	exit 1
fi

if [ ! -d $MINGWLIBS_PATH ]; then
	echo "clone mingwlibs git-repo"
	git clone $MINGWLIBS_REPO_URL $MINGWLIBS_PATH
fi

cd $MINGWLIBS_PATH

GITOUTPUT=$(git fetch 2>&1 |cat)
echo $GITOUTPUT
if [ -n "$GITOUTPUT" ];
then
	echo "removing builddir $BUILDDIR (mingwlibs updated)"
	rm -rf "$BUILDDIR"
	git clean -f -d
	git reset --hard origin/master
fi


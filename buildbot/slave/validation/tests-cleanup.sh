#!/bin/sh

set -e

. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests

#FIXME/HACK reset directory permissions
if [ -d ${TESTDIR}/usr/local/share/games/spring ]; then
	chmod 755 ${TESTDIR}/usr/local/share/games/spring
fi

#cleanup
rm -rf ${TESTDIR}


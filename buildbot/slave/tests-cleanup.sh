#!/bin/sh

set -e

. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests

#FIXME/HACK reset directory permissions
chmod 755 ${TESTDIR}/usr/local/share/games/spring

#cleanup
rm -rf ${TESTDIR}


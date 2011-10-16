#!/bin/sh

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests


if [ ! -n "$1" ]; then
	echo "Error in $0: no test script given"
	exit 1
fi

#run test
HOME=${TESTDIR} ${SOURCEDIR}/test/validation/run.sh ${TESTDIR}/usr/local/bin/spring-headless $@


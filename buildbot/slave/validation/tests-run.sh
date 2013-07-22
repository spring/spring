#!/bin/sh

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
SEVENZIP="7z a"

GAME=$2
MAP=$3
AI=$4
AIVER=$5
echo "Running test: GAME=$GAME MAP=$MAP AI=$AI AIVER=$AIVER"

if [ ! -n "$1" ]; then
	echo "Error in $0: no test script given"
	exit 1
fi

# don't abort on error
set +e
#run test
HOME=${TESTDIR} ${SOURCEDIR}/test/validation/run.sh ${TESTDIR}/usr/local/bin/spring-headless script.txt



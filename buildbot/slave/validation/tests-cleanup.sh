#!/bin/sh

set -e

. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests

#cleanup
rm -rf ${TESTDIR}


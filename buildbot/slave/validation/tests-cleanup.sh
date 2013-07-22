#!/bin/sh

# deletes all files created by the validation test
set -e

. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests

#cleanup
rm -rf ${TESTDIR}


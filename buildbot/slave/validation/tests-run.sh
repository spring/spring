#!/bin/sh

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
SEVENZIP="7z a"


if [ ! -n "$1" ]; then
	echo "Error in $0: no test script given"
	exit 1
fi

# don't abort on error
set +e
#run test
HOME=${TESTDIR} ${SOURCEDIR}/test/validation/run.sh ${TESTDIR}/usr/local/bin/spring-headless $@

# zip output for upload
mkdir -p ${TMP_PATH}/validation/
${SEVENZIP} ${TMP_PATH}/validation/$VERSION_$(date +"%Y-%m-%d_%H-%M-%S").7z ${TMP_BASE}/tests/.config/spring/
rm -rf ${TMP_BASE}/tests/.config/spring/


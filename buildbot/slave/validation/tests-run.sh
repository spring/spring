#!/bin/sh

# runs the validation test
set -e
. buildbot/slave/validation/tests-env.sh

# don't abort on error
set +e
#run test
HOME=${TESTDIR} ${SOURCEDIR}/test/validation/run.sh ${TESTDIR}/usr/local/bin/spring-headless script.txt



#!/bin/sh

# installs spring into the test dir
set -e
. buildbot/slave/validation/tests-env.sh

TESTDIR=${TMP_BASE}/tests


#install
cd ${BUILDDIR}
DESTDIR=${TESTDIR} ${MAKE} install-spring-headless install-pr-downloader demotool lua2php



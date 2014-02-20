#!/bin/sh

# installs spring into the test dir
set -e
. buildbot/slave/validation/tests-env.sh

TESTDIR=${TMP_BASE}/tests


#install
cd ${BUILDDIR}
DESTDIR=${TESTDIR} ${MAKE} pr-downloader_shared install-spring-headless install-pr-downloader demotool lua2php



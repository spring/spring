#!/bin/sh

# installs spring into the test dir
set -e
. buildbot/slave/validation/tests-env.sh

TESTDIR=${TMP_BASE}/tests


#install
cd ${BUILDDIR}
# FIXME: workarround for https://springrts.com/mantis/view.php?id=4716
${MAKE} generateVersionFiles
DESTDIR=${TESTDIR} ${MAKE} pr-downloader_shared install-spring-headless install-pr-downloader demotool lua2php



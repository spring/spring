#!/bin/bash

set -e
. buildbot/slave/prepare.sh

ulimit -t 600 -m 500000 -v 4000000

cd ${BUILDDIR}
export SPRING_DATADIR=${BUILDDIR}
$MAKE check
test/test_ThreadPool.exe --log_level=test_suite

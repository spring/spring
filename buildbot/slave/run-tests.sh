#!/bin/bash

set -e
. buildbot/slave/prepare.sh

NUM_CORES=1
which nproc &> /dev/null && NUM_CORES=`nproc`

set +e #maybe limit is already lower, do not abort if so
ulimit -t $((300 + NUM_CORES * 100))
ulimit -m 500000
#ulimit -v 4000000
set -e

cd ${BUILDDIR}
export SPRING_DATADIR=${BUILDDIR}
export WINEPATH="${BUILDDIR};${MINGWLIBS_PATH}/dll"
$MAKE check

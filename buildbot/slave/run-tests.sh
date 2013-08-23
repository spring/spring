#!/bin/bash

set -e
. buildbot/slave/prepare.sh

NUM_CORES=1
which nproc &> /dev/null && NUM_CORES=`nproc`
ulimit -t $((300 + NUM_CORES * 100)) -m 500000 -v 4000000

cd ${BUILDDIR}
export SPRING_DATADIR=${BUILDDIR}
export WINEPATH="${BUILDDIR};${MINGWLIBS_PATH}/dll"
$MAKE check

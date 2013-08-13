#!/bin/bash

set -e
. buildbot/slave/prepare.sh

ulimit -t 180 -m 500000 -v 4000000

cd ${BUILDDIR}
export SPRING_DATADIR=${BUILDDIR}
$MAKE check

#!/bin/bash

set -e
. buildbot/slave/prepare.sh

ulimit -t 180 -m 500000 -v 500000

cd ${BUILDDIR}
$MAKE check


#!/bin/sh

# Quit on error.
set -e

cd $(dirname $0)/..

CONFIG=${1}
BRANCH=${2}
MINGWLIBS_PATH=${3}
BUILDDIR=${PWD}/build-${CONFIG}
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}
REV=$(git describe --tags)

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi

./installer/make_installer.pl

mv ./installer/spring*.exe ${TMP_PATH}


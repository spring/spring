#!/bin/sh

# setups environment vars used in the validation test

. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
DOWNLOADDIR=${TMP_BASE}/download
CONTENT_DIR=${TESTDIR}/.config/spring

if [ ! -d test/validation/ ];
then
        echo "$0 has to be run from the source root dir"
        exit 1
fi

GAME=$1
MAP=$2
AI=$3
AIVER=$4

echo "Env: GAME=$GAME MAP=$MAP AI=$AI AIVER=$AIVER"


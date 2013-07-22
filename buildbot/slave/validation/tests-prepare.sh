#!/bin/bash

# installs spring-headless, downloads some content files and
# runs a simple high level tests of the engine (AI vs AI)

set -e
. buildbot/slave/validation/tests-env.sh

cd ${SOURCEDIR}

mkdir -p "${DOWNLOADDIR}"
mkdir -p "${CONTENT_DIR}"

PRDL="${TESTDIR}/usr/local/bin/pr-downloader --filesystem-writepath=$DOWNLOADDIR"
# get the name of the latest versions
GAME1=$($PRDL $GAME |egrep -o '\[Download\] (.*)' |cut -b 12-)
$PRDL "$MAP"

echo "Creating script: test/validation/prepare.sh \"$GAME1\" \"$MAP\" \"$AI\" \"$AIVER\""
${SOURCEDIR}/test/validation/prepare.sh "$GAME1" "$MAP" "$AI" "$AIVER" > ${CONTENT_DIR}/script.txt
${SOURCEDIR}/test/validation/prepare-client.sh ValidationClient 127.0.0.1 8452 >${CONTENT_DIR}/connect.txt


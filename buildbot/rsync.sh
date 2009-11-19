#!/usr/bin/env bash

set -e
cd $(dirname $0)/..

CONFIG=${1}
BRANCH=${2}
TMP_DIR=/tmp
REMOTE_HOST=localhost
REMOTE_BASE=/home/buildbot/www
REMOTE_PATH=${REMOTE_BASE}/${CONFIG}/${BRANCH}
REV=$(git describe --tags)
LOCAL_BASE=../build-${CONFIG}
BASE_ARCHIVE=${TMP_DIR}/${REV}_base.7z
EXE_ARCHIVE=${TMP_DIR}/${REV}_spring.7z
USYNC_ARCHIVE=${TMP_DIR}/${REV}_unitsync.7z

7z a ${BASE_ARCHIVE} ${LOCAL_BASE}/base
7z a ${EXE_ARCHIVE} ${LOCAL_BASE}/spring.exe
7z a ${USYNC_ARCHIVE} ${LOCAL_BASE}/unitsync.dll

rsync -lrvz ${BASE_ARCHIVE} ${REMOTE_HOST}:${REMOTE_PATH}/${BASE_ARCHIVE}
rsync -lrvz ${EXE_ARCHIVE} ${REMOTE_HOST}:${REMOTE_PATH}/${EXE_ARCHIVE}
rsync -lrvz ${USYNC_ARCHIVE} ${REMOTE_HOST}:${REMOTE_PATH}/${USYNC_ARCHIVE}

rm -f ${BASE_ARCHIVE}
rm -f ${EXE_ARCHIVE}
rm -f ${USYNC_ARCHIVE}



#!/bin/sh
set -e

# this script is already called from tests-analyze.sh, so no need to call it here
#. buildbot/slave/prepare.sh

REMOTE_HOST=springrts.com
REMOTE_USER=buildbot
REMOTE_BASE=/home/buildbot/www
RSYNC="rsync -vzr --chmod=D+rx,F+r --bwlimit 300"
REMOTE_RSYNC="nice -19 ionice -c3 rsync" #prevent QQ about rsync killing server

${RSYNC} ${TMP_BASE}/tests/.spring/ ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/${CONFIG}/${BRANCH}/${REV}/validation/

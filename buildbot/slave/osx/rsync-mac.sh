#!/bin/bash
set -e
. buildbot/slave/prepare.sh

#TODO: this is the same as in rsync.sh

REMOTE_HOST=springrts.com
REMOTE_USER=buildbot
REMOTE_BASE=/home/buildbot/www
RSYNC="rsync -avz --chmod=D+rx,F+r --bwlimit 50"
REMOTE_RSYNC="nice -19 ionice -c3 rsync" #prevent QQ about rsync killing server

umask 022

#cleanup installed files before rsyncing
rm -rf ${TMP_BASE}/inst/

# Rsync archives to a world-visible location.
if [ ${REMOTE_HOST} = localhost ] && [ -w ${REMOTE_BASE} ]; then
	${RSYNC} ${TMP_BASE}/ ${REMOTE_BASE}/
else
	${RSYNC} --rsync-path="${REMOTE_RSYNC}" ${TMP_BASE}/ ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/
fi

# Clean up.
rm -rf ${TMP_BASE}/*

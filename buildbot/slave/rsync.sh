#!/bin/bash
set -e
. buildbot/slave/prepare.sh

REMOTE_HOST=springrts.com
REMOTE_USER=buildbot
REMOTE_BASE=/home/buildbot/www
BWLIMIT=4000
if [ "$OUTPUTDIR" == "win64" ] ; then
	BWLIMIT=1000
fi
RSYNC="rsync -avz --chmod=D+rx,F+r --bwlimit $BWLIMIT --exclude=download/ --exclude=tests/ --remove-source-files"
REMOTE_RSYNC="nice -19 ionice -c3 rsync" #prevent QQ about rsync killing server

if [ -z "${TMP_BASE}" ]; then
	echo "TMP_BASE is empty, something went wrong!"
	exit 1
fi

umask 022

# use old files as base to reduce upload
# Commented out cause it doesn't work. Binaries & debugsymbols just don't share data between compiles :<
#RSYNC="${RSYNC} --fuzzy"
#RSYNC="${RSYNC} --compare-dest=\"${REMOTE_BASE}/${CONFIG}/${BRANCH}/LATEST_LINUX_STATIC/\""

# cleanup installed files before rsyncing
rm -rf ${TMP_BASE}/inst/

# Rsync archives to a world-visible location.
if [ ${REMOTE_HOST} = localhost ] && [ -w ${REMOTE_BASE} ]; then
	${RSYNC} ${TMP_BASE}/ ${REMOTE_BASE}/
else
	${RSYNC} --rsync-path="${REMOTE_RSYNC}" ${TMP_BASE}/ ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_BASE}/
fi

# delete empty dirs (--remove-source-files doesn't remove dirs)
find ${TMP_BASE}/ -depth -type d -empty -delete


#!/bin/sh

set -e
. buildbot/slave/prepare.sh

umask 022

REMOTE_PATH=OmoikaneWeb:/mnt/data/WWW/spring.osocial.se/
RSYNC="rsync -avz --delete-after"

${RSYNC} ${SOURCEDIR}/doc/doxygen/html/ ${REMOTE_PATH}


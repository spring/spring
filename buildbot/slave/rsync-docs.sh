#!/bin/sh

set -e
. buildbot/slave/prepare.sh

umask 022

#FIXME: we should somehow delete the source-directory, as it will grow over time

REMOTE_PATH=OmoikaneWeb:/mnt/data/WWW/spring.osocial.se/
RSYNC="rsync -az --delete-after"

${RSYNC} ${SOURCEDIR}/doc/doxygen/html/ ${REMOTE_PATH}


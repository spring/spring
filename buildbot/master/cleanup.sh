#!/bin/sh
# Removes non-release builds older than ${AGE} days,
# and debug symbols older than ${DBG_AGE}.
# ${DBG_AGE} must be smaller than or equal to ${AGE}.

AGE=180
DBG_AGE=30
ZIP_AGE=10

renice 19 -p $$ >/dev/null
ionice -c3 -p $$

RM="rm"
RMDIR="rmdir"

# Uncomment this for dry run.
#RM=echo
#RMDIR=echo
BUILDBOTDIR=/home/buildbot/www/default

#old debug files
find ${BUILDBOTDIR} -type f -not -path '*/master/*' -path "*dbg.7z*" -mtime +${DBG_AGE} -exec ${RM} '{}' \;
#old zip (only needed for zk)
find ${BUILDBOTDIR} -type f -not -path '*/master/*' -path "*minimal-portable+dedicated.zip" -mtime +${ZIP_AGE} -exec ${RM} '{}' \;

#very old files
find ${BUILDBOTDIR} -type f -not -path '*/master/*' -mtime +${AGE} -exec ${RM} '{}' \;
#broken symbolic links
find ${BUILDBOTDIR} -type l -xtype l -not -path '*/master/*' -exec ${RM} '{}' \;
#empty directories
find ${BUILDBOTDIR} -ignore_readdir_race -type d -empty -exec ${RMDIR} '{}' \;


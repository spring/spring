#!/bin/sh
# Removes non-release builds older than ${AGE} days,
# and debug symbols older than ${DBG_AGE}.
# ${DBG_AGE} must be smaller than or equal to ${AGE}.

AGE=90
DBG_AGE=14

renice 19 -p $$ >/dev/null
ionice -c3 -p $$

RM="rm"
RMDIR="rmdir"

# Uncomment this for dry run.
#RM=echo
#RMDIR=echo

#old debug files
find /home/buildbot/www -type f -not -path '*/master/*' -path "*dbg.7z*" -mtime +${DBG_AGE} -exec ${RM} '{}' \;
#very old files
find /home/buildbot/www -type f -not -path '*/master/*' -mtime +${AGE} -exec ${RM} '{}' \;
#empty directories
find /home/buildbot/www -ignore_readdir_race -type d -empty -exec ${RMDIR} '{}' \;


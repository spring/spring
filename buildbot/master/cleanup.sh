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

find /home/buildbot/www -mindepth 4 -type f -regex '.*-g[a-f0-9]+[_.].*_dbg\.7z' -mtime +${DBG_AGE} -exec ${RM} '{}' \;
find /home/buildbot/www -mindepth 4 -type f -regex '.*-g[a-f0-9]+[_.].*' -mtime +${AGE} -exec ${RM} '{}' \;
find /home/buildbot/www -ignore_readdir_race -maxdepth 3 -type d -empty -exec ${RMDIR} '{}' \;

#!/bin/sh
# Removes non-release builds older than 90 days.
find /home/buildbot/www -mindepth 4 -maxdepth 4 -type f -regex '.*-g[a-f0-9]+[_.].*' -mtime +90 -exec rm -v '{}' \;
find /home/buildbot/www -ignore_readdir_race -maxdepth 3 -type d -empty -exec rmdir -v '{}' \;

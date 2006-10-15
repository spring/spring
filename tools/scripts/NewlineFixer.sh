#!/bin/sh
# Author: Tobi Vollebregt

# Use this script to fix 'warning: no newline at end of file' warnings.
# Usage: buildtool -k 2>&1 | NewlineFixer.sh
# replace buildtool with whatever you use, e.g. make, scons, etc.
# use -k flag on buildtool to keep going after errors.

tmp=.NewlineFixer.tmp

grep 'warning: no newline at end of file' | sed 's/:.*//g' | sort | uniq > "$tmp"

for f in $(cat "$tmp" | sed 's/ /___/g')
do
	g=$(echo "$f" | sed 's/___/ /g')
	echo >> "$g"
done

rm $tmp

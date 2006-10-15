#!/bin/sh
# Author: Tobi Vollebregt

# Use to fix errors like these (occuring with gcc >= 4.1):
# error: extra qualification 'CSpotFinder::' on member 'GetBestCachePoint'
# Pipe output of build process into it, ie.
#	buildtool -k | ./ExtraQualFixer.sh
# Use -k to keep build going on errors.
# This way all extra qualification errors are fixed in one pass.
# Replace buildtool with scons, make, whatever you use.

tmp=.ExtraQualFixer.tmp
grep 'error: extra qualification ' | sort | uniq > "$tmp"

IFS='
'
for line in $(cat "$tmp");
do
	# figure out symbol name, with and without extra qualification
	symbolwq=$(echo "$line" | awk -F \' '{print $2 $4}')
	symbolwoq=$(echo "$symbolwq" | sed 's/.*:://g')
	# figure out the filename
	file=$(echo "$line" | awk -F ':' '{print $1}')
	# replace symbol with extra qual. with symbol without extra qual.
	sed -i "s/$symbolwq/$symbolwoq/g" "$file"
done
rm "$tmp"

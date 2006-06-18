#!/bin/sh
# Author: Tobi Vollebregt

# Run from trunk. Sets svn:eol-style property to native on all files.
# Binary files are automagically ignored because svn errors out on them.

files=$(svn status -v | egrep -v "^\?" | awk '{ print $4 }' | sed "s/ /___/g")

for f in $files; do
	g=$(echo "$f" | sed "s/___/ /g")
	[ -f "$g" ] && svn propset svn:eol-style native "$g"
done

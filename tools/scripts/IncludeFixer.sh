#!/bin/sh
# Author: Tobi Vollebregt

# Use this script to fix #include directives (and other places where filenames occur)
# that have the wrong case.
# Usage example: ./IncludeFixer.sh *.cpp *.h

for f
do
	g=$(echo $f | sed 's/\./\\./g')
	sed -i "s/$g/$f/gi" "$@"
done

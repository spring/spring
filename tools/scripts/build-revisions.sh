#!/bin/sh
# Author: Tobi Vollebregt
#
# This script can be used to build every revision in a certain range.
# Usage: ./build-revisions.sh revision1 revision2 options
#
# It starts at revision1, builds spring, goes up one revision, if there were
# any files in rts/ with .c* or .h* extension modified, it builds spring for
# that revision. Then it goes up another revision, etc. etc., until it
# reaches revision2.
#
# Very useful to find a regression, using this you can pre-build all
# revisions and do a binary search later on the readily available executables.
#
# Extra options after revision2 are passed literally to scons configure.



# Configuration

logdir="logs"
cachedir="cache"



# Some sanity checks

quit=0

if [ x$(( $# < 2 )) == x1 ]; then
	echo "Usage: $0 revision1 revision2"
	quit=1
fi

if [ ! -d "$logdir" ]; then
	echo "'$logdir' directory does not exist"
	quit=1
fi

if [ "$quit" != "0" ]; then
	exit "$quit"
fi



# Which revisions to build?

left=$1
right=$2
shift 2



# Start building

echo "Building revisions $left..$right !"
echo "Number of builds : $(( $right - $left + 1 ))"

for (( r=$left ; $r<=$right ; r=$r+1 )); do
	echo "* Building revision $r ..."
	logfile="$logdir/log-r$r.txt"
	( svn up --non-interactive -r "$r" 2>&1 || ( echo "Subversion failure.  Stop."; exit 1; ) ) | tee $logfile
	if [ "$r" == "$left" ] || [ "x$(egrep "rts/.*\.[ch]" "$logfile")" != "x" ]; then
		scons -c
		echo "* scons configure cachedir=$cachedir $@ && scons spring" | tee -a $logfile
		( scons configure "cachedir=$cachedir" "$@" && scons spring && mv "game/spring" "game/spring-r$r") 2>&1 | tee -a $logfile
	fi
done

echo "Done building revisions $left..$right !"

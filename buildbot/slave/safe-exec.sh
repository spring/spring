#!/bin/sh


if [ $# -le 1 ]; then
	echo Usage: $0 /path/to/binary [params]
	exit 1
fi

if [ -f $1 ] && [ -x $1 ]; then
	TOEXEC=$1
	shift 1
	exec $TOEXEC $@
else
	echo $1 isn't executable / doesn't exist
fi


#!/bin/sh

set -e #abort on error

if [ $# -le 0 ]; then
	echo "Usage: $0 /path/to/spring [parameters]"
	exit 1
fi


if [ ! -x "$1" ]; then
	echo "Parameter 1 $1 isn't executable!"
	exit 1
fi

GDBCMDS=$(mktemp)
(
	echo file $1
	# set breakpoints + log to stderr when breakpoint is hit
	for condition in CrashHandler::HandleSignal __assert_fail; do
		echo break $condition
		echo command
		echo	set logging file /dev/stderr
		echo	set logging on
		echo	bt full
		echo	quit
		echo end
	done
	echo run $2 $3 $4 $5 $6 $7 $8 $9
	echo if sig != 0
	echo   set logging file /dev/stderr
	echo   set logging on
	echo   bt full
	echo end
	echo quit
)>$GDBCMDS

#limit to 1GB RAM
ulimit -v 1000000
#max 15 min cpu time
ulimit -t 900

set +e #temp disable abort on error
gdb -batch-silent -return-child-result -x $GDBCMDS
#store exit code
EXIT=$?
set -e

#cleanup
rm -f $GDBCMDS
exit $EXIT


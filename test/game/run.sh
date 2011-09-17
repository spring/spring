#!/bin/sh

set -e #abort on error


if [ $# -le 0 ]; then
	echo "Usage: $0 /path/to/spring [parameters]"
	exit 1
fi

if [ -n $LOG ]; then #LOG not set, use default
	DATESTR=$(date +"%Y.%m.%d_%H-%M-%S")
	LOG=$DATESTR.log
fi
GDBCMDS=$LOG.gdbcmds

if [ ! -x "$1" ]; then
	echo "Parameter 1 $1 isn't executable!"
	exit 1
fi

(
echo file $1
echo set logging on $LOG
echo set logging redirect off
echo run $2 $3 $4 $5 $6 $7 $8 $9
echo bt full
echo quit
)>$GDBCMDS

#limit to 1GB RAM
ulimit -v 1000000
#max 15 min cpu time
ulimit -t 900
echo Starting Test, logging to $LOG ...

set +e #temp disable abort on error
gdb -batch-silent -x $GDBCMDS >$LOG 2>&1

ERROR=0
set -e

if [ ! $? -eq 0 ]; then
	ERROR=1
fi

if ! grep " exited normally." $LOG >/dev/null; then
	ERROR=1
fi

if [ $ERROR -eq 0 ]; then
	echo ok
else
	echo failed
fi

cat $LOG

#cleanup
rm -f $GDBCMDS
rm -f $LOG

exit $ERROR

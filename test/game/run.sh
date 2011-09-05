#!/bin/sh

set -e #abort on error


if [ $# -le 0 ]; then
	echo "Usage: $0 /path/to/spring [parameters]"
	exit 1
fi

if [ -n $LOG ]; then #LOG not set, use default
	DATESTR=$(date +"%Y.%m.%d_%H-%M-%S")
	LOGDIR=logs
	LOG=$LOGDIR/$DATESTR.log
fi
GDBCMDS=$LOG.gdbcmds

if [ ! -x "$1" ]; then
	echo "Parameter 1 $1 isn't executable!"
	exit 1
fi

mkdir -p $LOGDIR
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
echo -n Starting Test, logging to $LOG ...

set +e #temp disable abort on error
gdb -batch-silent -x $GDBCMDS >$LOG 2>&1
if [ ! $? -eq 0]; then; #gdb exited abnormally, dump output
	cat $LOG
	exit 1
fi
set -e

if ! grep "Program exited normally." $LOG >/dev/null; then
	echo ": failed"
	#echo logoutput to stderr
	cat $LOG
	exit 1
else
	echo ": ok"
fi


#cleanup (this info is alread in normal log)
rm -f $GDBCMDS

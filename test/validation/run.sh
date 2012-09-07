#!/bin/sh

set -e # abort on error

Analyzecoredump() {

SPRING=$1
COREFILE=$2

if [ -s $COREFILE ]; then
	echo Core file found, creating backtrace
	GDBCMDS=$(mktemp)
	(
		echo file $SPRING
		echo core-file $COREFILE
		echo bt full
		echo quit
	)>$GDBCMDS
	gdb -batch -x $GDBCMDS
	cat $GDBCMDS
	# cleanup
	rm -f $GDBCMDS
	rm -f $COREFILE
fi

}

if [ $# -le 0 ]; then
	echo "Usage: $0 /path/to/spring testScript [parameters]"
	exit 1
fi


if [ ! -x "$1" ]; then
	echo "Parameter 1 $1 isn't executable!"
	exit 1
fi

if [ "$(cat /proc/sys/kernel/core_pattern)" != "core" ]; then
	echo "Please set /proc/sys/kernel/core_pattern to core"
	exit 1
fi

if [ "$(cat /proc/sys/kernel/core_pattern)" != "core" ]; then
	echo "Please run sudo echo core >/proc/sys/kernel/core_pattern"
	exit 1
fi

if [ "$(cat /proc/sys/kernel/core_uses_pid)" != "1" ]; then
	echo "Please run sudo echo 1 >/proc/sys/kernel/core_uses_pid"
	exit 1
fi

# enable core dumps
ulimit -c unlimited

RUNCLIENT=test/validation/run-client.sh

if [ ! -x $RUNCLIENT ]; then
	echo "$RUNCLIENT doesn't exist, please run from the source-root directory"
	exit 1
fi


# limit to 1GB RAM
ulimit -v 1000000
# max 15 min cpu time
ulimit -t 900

# FIXME: remove old caching files, the client-script would start immediately when they exist
# maybe a foreign directory for the client is the cleanest way...
rm -rf ~/.spring/cache/paths

# start up the client in background
$RUNCLIENT $1 &
PID_CLIENT=$!

set +e #temp disable abort on error
$@ &
PID_HOST=$!

# auto kill host after 15mins
#sleep 900 && kill -9 $PID_HOST &

echo waiting for host to exit
# store exit code
wait $PID_HOST
EXIT=$?
Analyzecoredump $1 $HOME/.spring/core.$PID_HOST

echo waiting for client to exit
# get spring client process exit code / wait for exit
wait $PID_CLIENT
EXITCHILD=$?
Analyzecoredump $1 $HOME/.spring/core.$PID_CLIENT

#reenable abbort on error
set -e

echo Server and client exited


# wait for client to exit
if [ $EXITCHILD -ne 0 ];
then
	echo Client exited with $EXITCHILD
	exit $EXITCHILD
fi

exit $EXIT


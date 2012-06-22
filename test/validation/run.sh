#!/bin/sh

set -e # abort on error

if [ $# -le 0 ]; then
	echo "Usage: $0 /path/to/spring testScript [parameters]"
	exit 1
fi


if [ ! -x "$1" ]; then
	echo "Parameter 1 $1 isn't executable!"
	exit 1
fi

RUNCLIENT=test/validation/run-client.sh

if [ ! -x $RUNCLIENT ]; then
	echo "$RUNCLIENT doesn't exist, please run from the source-root directory"
	exit 1
fi

GDBCMDS=$(mktemp)
(
	echo file $1
	echo run $2 $3 $4 $5 $6 $7 $8 $9
	echo bt full
	echo quit
)>$GDBCMDS

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
#gdb -batch -return-child-result -x $GDBCMDS &
$@ &

PID_HOST=$!

# auto kill host after 15mins
#sleep 900 && kill -9 $PID_HOST &

# store exit code
wait $PID_HOST
EXIT=$?
set -e

# cleanup
rm -f $GDBCMDS

# get spring client process exit code / wait for exit
wait $PID_CLIENT
EXITCHILD=$?
# wait for client to exit
if [ $EXITCHILD -ne 0 ];
then
	echo "Error: Spring-client exited with error code $EXITCHILD"
	exit $EXITCHILD
fi
exit $EXIT


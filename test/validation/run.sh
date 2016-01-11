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

if [ "$(cat /proc/sys/kernel/core_pattern)" != "core" ]; then
	echo "Please set /proc/sys/kernel/core_pattern to core"
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


# limit to 1.5GB RAM
#ulimit -v 1500000
# max 4 min cpu time
ulimit -t 240

# delete path cache
rm -rf ~/.config/spring/cache/

# start up the client in background
$RUNCLIENT "$1 --nocolor" &
PID_CLIENT=$!

# start host
echo "Starting Host"
set +e #temp disable abort on error
$@ --nocolor &
PID_HOST=$!

# auto kill host after 15mins
#sleep 900 && kill -9 $PID_HOST &

echo waiting for host to exit, pid: $PID_HOST
# store exit code
wait $PID_HOST
EXIT=$?

echo waiting for client to exit, pid: $PID_CLIENT
# get spring client process exit code / wait for exit
wait $PID_CLIENT
EXITCHILD=$?

#reenable abbort on error
set -e

# exit with exit code of server/client if failed
if [ $EXITCHILD -ne 0 ];
then
	echo Client exited with $EXITCHILD
	exit $EXITCHILD
fi

exit $EXIT


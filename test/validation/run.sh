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

# start up the client in background
#$(pwd)/$(dirname $0)/run-client.sh $1 &

set +e #temp disable abort on error
gdb -batch -return-child-result -x $GDBCMDS
# store exit code
EXIT=$?
set -e

# cleanup
rm -f $GDBCMDS

# wait for client to exit
#wait
exit $EXIT


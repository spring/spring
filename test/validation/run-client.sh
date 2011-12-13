#!/bin/bash


set -e #abort on error

if [ $# -ne 1 ]; then
	echo "runs spring as client, waits until  ~/.spring/cache/paths/*.pe[1|2].zip exists"
	echo "Usage: $0 /path/to/spring"
	exit 1
fi

CACHEDIR=~/.spring/cache/paths
HEADLESS=$1
MAXWAIT=60

for (( i=0; $i<$MAXWAIT; i++ ));
do
	if [ -s $CACHEDIR/*.pe.zip ] && [ -s $CACHEDIR/*.pe2.zip ];
	then
		# wait additional seconds to let the other process write the files
		sleep 3
		LOG=$(mktemp)
		echo "Starting $HEADLESS client"
		set +e
		$HEADLESS connect.txt &>$LOG
		EXIT=$?
		# dump log file at exit, to not mix client + server output
		# FIXME: this merges stdout + stderr
		echo "=========== Dump of client log file start"
		cat $LOG
		echo "=========== Dump of client log file end"
		set -e
		rm -f $LOG
		exit $EXIT
	fi
	# don't use 100% cpu in polling
	sleep 1

done

echo "cache file didn't show up within MAXWAIT=$MAXWAIT seconds"
exit 1


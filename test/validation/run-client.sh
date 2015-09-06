#!/bin/bash


set -e #abort on error

if [ $# -ne 1 ]; then
	echo "runs spring as client, waits until  ~/.config/spring/cache/paths/*.pe[1|2].zip exists"
	echo "Usage: $0 /path/to/spring"
	exit 1
fi

HEADLESS=$1
MAXWAIT=90

for (( i=0; $i<$MAXWAIT; i++ ));
do
	if [ -s ~/.config/spring/infolog.txt ] && [ -n "$(grep "finalized P" ~/.config/spring/infolog.txt)" ];
	then
		sync
		sleep 1
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


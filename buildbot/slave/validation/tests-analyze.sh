#!/bin/sh

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
SPRING=${TESTDIR}/usr/local/bin/spring-headless

if ! [ -x $SPRING ]; then
	echo Usage: $0 /path/to/spring
	exit 1
fi

EXITCODE=0

for i in $(find ${TESTDIR}/.spring -maxdepth 1 -type f -name "core.*") ; do

	COREFILE=$i

	echo Core file found, creating backtrace
	GDBCMDS=$(mktemp)
	(
		echo file $SPRING
		echo core-file $COREFILE
		echo info program
		echo bt full
		echo quit
	)>$GDBCMDS
	gdb -batch -x $GDBCMDS
	cat $GDBCMDS
	# cleanup
	rm -f $GDBCMDS
	rm -f $COREFILE
	EXITCODE=1
done
if [ $EXITCODE -ne 0 ]; then
	echo Tests failed, uploading spring writeable dir
	mkdir -p ${TMP_PATH}/validation/
	mv ${TMP_BASE}/tests/.spring/ ${TMP_PATH}/validation/
fi
exit $EXITCODE

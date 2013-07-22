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

for COREFILE in $(find ${TESTDIR}/.config/spring ${TESTDIR}/ -maxdepth 1 -type f -name "core.*")
do
	echo Core file found, creating backtrace
	GDBCMDS=$(mktemp)
	(
		echo file $SPRING
		echo core-file $COREFILE
		echo info program
		echo bt full
		echo quit
	) > $GDBCMDS
	gdb -batch -x $GDBCMDS
	cat $GDBCMDS
	# cleanup
	rm -f $GDBCMDS
	rm -f $COREFILE
	EXITCODE=1
done

# zip output for upload
mkdir -p ${TMP_PATH}/validation/
${SEVENZIP} ${TMP_PATH}/validation/$VERSION_$(date +"%Y-%m-%d_%H-%M-%S")-dbg.7z ${TMP_BASE}/tests/.config/spring/
rm -rf ${TMP_BASE}/tests/.config/spring/

exit $EXITCODE

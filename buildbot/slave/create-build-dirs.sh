#!/bin/sh

# Quit on error.
set -e

DIR=build-${1}
shift
echo -n "creating ${DIR} ... "

if [ ! -d ${DIR} ] ; then
	mkdir ${DIR}
	echo "done."
else
	echo "skipped."
fi

echo "configuring ${DIR} with $@ ..."

cd ${DIR}
cmake .. $@

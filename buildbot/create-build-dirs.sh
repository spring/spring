#!/usr/bin/env bash

set -e
cd $(dirname ${0})/..

DIR=build-${1}
shift
echo -n creating ${DIR} ...

if [ ! -d ${DIR} ] ; then
	mkdir ${DIR}
	echo done
else
	echo skipped
fi

echo -n configuring ${DIR} with $@ ...

cd ${DIR}
cmake ..  $@


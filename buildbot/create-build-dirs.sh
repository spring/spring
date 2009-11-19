#!/usr/bin/env bash

set -e
cd $(dirname ${0})/..

DIR=build-${1}

echo -n creating ${DIR} ...

if [ ! -d ${DIR} ] ; then
	mkdir ${DIR}
	echo done
else
	echo skipped
fi

echo -n configuring ${DIR} with ${4} ...

cd ${DIR}
cmake .. -DCMAKE_TOOLCHAIN_FILE:STRING=${2} -DMINGWLIBS:PATH=${3}  ${4}


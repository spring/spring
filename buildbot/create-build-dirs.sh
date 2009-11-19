#!/usr/bin/env bash

set -e
cd $(dirname $0)/..

echo -n creating $1 ...
(
if [ ! -d $1 ] ; then
	mkdir $1
	echo done
else
	echo skipped
fi
)
echo -n configuring $1 with $4 ...
(
cd $1
cmake .. -DCMAKE_TOOLCHAIN_FILE:STRING=$2 -DMINGWLIBS:PATH=$3  $4
)

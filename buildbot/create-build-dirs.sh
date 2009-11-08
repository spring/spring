#!/usr/bin/env bash

set -e
cd $(dirname $0)/..

create-build-dir-cmake ()
{
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
}

create-build-dir-cmake build-MT $1 $2 "-DUSE_GML_SIM=true -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DUSE_GML=true"
create-build-dir-cmake build-MTDEBUG $1 $2 "-DUSE_GML_SIM=true -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DUSE_GML_DEBUG=true -DUSE_GML=true"
create-build-dir-cmake build-debug2 $1 $2 "-DCMAKE_BUILD_TYPE=DEBUG2"
create-build-dir-cmake build-default $1 $2 "-DCMAKE_BUILD_TYPE=RELWITHDEBINFO"
create-build-dir-cmake build-profiling $1 $2 "-DCMAKE_BUILD_TYPE=PROFILE"
create-build-dir-cmake build-syncdebug $1 $2 "-DCMAKE_BUILD_TYPE=DEBUG2 -DTRACE_SYNC=true -DSYNCDEBUG=true"
create-build-dir-cmake build-syncdebug2 $1 $2 "-DCMAKE_BUILD_TYPE=DEBUG2 -DSYNCDEBUG=true"

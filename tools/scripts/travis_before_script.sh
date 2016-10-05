#!/bin/sh

set -e

if [ "$CXX" = "g++" ]; then
	cmake -DCMAKE_BUILD_TYPE=DEBUG -DDEBUG_MAX_WARNINGS=OFF -DCMAKE_C_FLAGS=-gtoggle -DCMAKE_CXX_FLAGS=-gtoggle .
else
	export CCACHE_CPP2=yes
	cmake -DCMAKE_BUILD_TYPE=DEBUG -DDEBUG_MAX_WARNINGS=OFF .
fi

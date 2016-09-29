#!/bin/sh

set -e

if [ "$CXX" = "g++" ]; then
	cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS=-gtoggle -DCMAKE_CXX_FLAGS=-gtoggle .
else
	cmake -DCMAKE_BUILD_TYPE=DEBUG .
fi

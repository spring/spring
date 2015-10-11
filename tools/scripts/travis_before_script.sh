#!/bin/sh

set -e

if [ "$CXX" = "g++" ]; then
	cmake -DCMAKE_C_FLAGS=-gtoggle -DCMAKE_CXX_FLAGS=-gtoggle .
else
	cmake .
fi

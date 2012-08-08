#!/bin/bash

BINARY=${1:-build/default/spring}

echo "Direct linked libraries: "
echo "readelf -d ${BINARY}: "
readelf -d ${BINARY} | grep "Shared library:" | awk '{print $3" "$4" "$5;}'
echo " "
echo " "
echo "Indirect linked libraries: "
echo "ldd ${BINARY}: "
ldd ${BINARY}
exit $?

#!/bin/bash

BINARY=${1:-build/default/spring}
LIBRARY=${2}

# check if the binary links the library
readelf -d ${BINARY} | grep -i "Shared library:" | grep -iq ${LIBRARY}

# grep returns 0 if found and 1 if not
RESULT=$?

# invert the return argument (it should give an error when the library is linked)
if (( RESULT == 1 )); then
	echo "${BINARY} static linked to ${LIBRARY}"
	exit 0
elif (( RESULT == 0 )); then
	echo "${BINARY} links to ${LIBRARY}"
	exit 1
fi
exit $?

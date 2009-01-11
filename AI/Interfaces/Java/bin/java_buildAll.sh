#!/bin/sh
#
# Go through the whole Java build process
# @param_1	interface version (eg: "0.1")
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
cd ${THIS_DIR}

if [ $# != 1 ]
then
	echo "Usage: ${0} [java-interface-version]" 1>&2
	exit 1
fi
INTERFACE_VERSION=${1}

sh ./java_generateWrappers.sh
sh ./java_compile.sh
sh ./java_install.sh ${INTERFACE_VERSION}

cd ${CWD_BACKUP}


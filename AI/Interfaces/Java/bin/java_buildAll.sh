#!/bin/sh
#
# Go through the whole Java build process
# @param_1	interface version (eg: "0.1")
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})

if [ $# != 2 ]
then
	echo "Usage: ${0} [java-interface-version] [spring-data-dir]" 1>&2
	exit 1
fi
INTERFACE_VERSION=${1}
SPRING_DATA_DIR=${2}

##############################################
### do not change anything below this line ###

sh ${THIS_DIR}/java_generateWrappers.sh
sh ${THIS_DIR}/java_compile.sh
sh ${THIS_DIR}/java_install.sh ${INTERFACE_VERSION} ${SPRING_DATA_DIR}

cd ${CWD_BACKUP}


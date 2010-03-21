#!/bin/sh
#
# Move the Java Part of the interface to its final destination.
# @param	interface version (eg: "0.1")
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
HOME_DIR=$(cd ${THIS_DIR}; cd ..; pwd)

if [ $# != 2 ]
then
	echo "Usage: ${0} [java-interface-version] [spring-data-dir]" 1>&2
	exit 1
fi
INTERFACE_VERSION=${1}
SPRING_DATA_DIR=${2}

#SPRING_HOME=$(cd ${HOME_DIR}; cd ../../..; pwd)
#SPRING_DATA_DIR=${SPRING_HOME}/dist

##############################################
### do not change anything below this line ###

echo "	installing ..."
mkdir -p ${SPRING_DATA_DIR}/AI/Interfaces/Java/${INTERFACE_VERSION}/jlib
echo cp ${HOME_DIR}/interface.jar ${SPRING_DATA_DIR}/AI/Interfaces/Java/${INTERFACE_VERSION}
echo cp ${HOME_DIR}/interface-src.jar ${SPRING_DATA_DIR}/AI/Interfaces/Java/${INTERFACE_VERSION}

cd ${CWD_BACKUP}


#!/bin/sh
#
# Move the Java Part of the interface to its final destination.
# @param	interface version (eg: "0.1")
#

CWD_BACKUP=`pwd`
THIS_DIR=`dirname ${0}`
cd ${THIS_DIR}

if [ $# != 1 ]
then
	echo "Usage: ${0} [java-interface-version]" 1>&2
	exit 1
fi
INTERFACE_VERSION=${1}

MY_HOME=..
SPRING_HOME=${MY_HOME}/../../..

echo "	installing ..."
mkdir -p ${SPRING_HOME}/game/AI/Interfaces/Java/${INTERFACE_VERSION}/jlib
cp ${MY_HOME}/interface.jar ${SPRING_HOME}/game/AI/Interfaces/Java/${INTERFACE_VERSION}

cd ${CWD_BACKUP}


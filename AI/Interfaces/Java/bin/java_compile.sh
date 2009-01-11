#!/bin/sh
#
# Compiles and packages the Java sources
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
HOME_DIR=$(cd ${THIS_DIR}; cd ..; pwd)

JAVA_PKG_FIRST_PART=com
JAVA_PKG=${JAVA_PKG_FIRST_PART}/clan_sy/spring/ai

##############################################
### do not change anything below this line ###

JLIB_DIR="${HOME_DIR}/data/jlib"
JAVA_SRC_FILES="${JAVA_PKG}/*.java ${JAVA_PKG}/command/*.java ${JAVA_PKG}/event/*.java ${JAVA_PKG}/oo/*.java"
SOURCE_DIR=${HOME_DIR}/java/src
BUILD_DIR=${HOME_DIR}/build

echo "	compiling ..."
mkdir -p ${BUILD_DIR}
cd ${SOURCE_DIR}
javac -cp ".:${JLIB_DIR}/jna/jna.jar:${JLIB_DIR}/vecmath.jar" -d "${BUILD_DIR}" ${JAVA_SRC_FILES}

echo "	packaging ..."
cd ${BUILD_DIR}
jar cmf ${SOURCE_DIR}/manifest.mf ${HOME_DIR}/interface.jar ${JAVA_PKG_FIRST_PART}
jar cf ${HOME_DIR}/interface-src.jar -C "${SOURCE_DIR}" ${JAVA_PKG_FIRST_PART}
rm -Rf ${BUILD_DIR}
echo "	done."

cd ${CWD_BACKUP}


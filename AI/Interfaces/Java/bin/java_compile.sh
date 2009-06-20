#!/bin/sh
#
# Compiles and packages the Java sources
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
HOME_DIR=$(cd ${THIS_DIR}; cd ..; pwd)

JAVA_PKG_FIRST_PART=com
JAVA_PKG=${JAVA_PKG_FIRST_PART}/springrts/ai

##############################################
### do not change anything below this line ###

JLIB_DIR="${HOME_DIR}/data/jlib"
JAVA_SRC_FILES="${JAVA_PKG}/*.java ${JAVA_PKG}/command/*.java ${JAVA_PKG}/event/*.java ${JAVA_PKG}/oo/*.java"

SOURCE_MAIN_DIR=${HOME_DIR}/java/src
if [ -z "${SOURCE_GENERATED_DIR}" ]; then
	SOURCE_GENERATED_DIR=${HOME_DIR}/java/generated
fi
if [ -z "${BUILD_DIR}" ]; then
	BUILD_DIR=${HOME_DIR}/build
fi
if [ -z "${BIN_DIR}" ]; then
	BIN_DIR=${HOME_DIR}
fi

echo "	compiling ..."
mkdir -p ${BUILD_DIR}
cd ${SOURCE_GENERATED_DIR}
javac -cp ".:${JLIB_DIR}/jna/jna.jar:${JLIB_DIR}/vecmath.jar:${SOURCE_MAIN_DIR}" -d "${BUILD_DIR}" ${JAVA_SRC_FILES}

echo "	packaging ..."
cd ${BUILD_DIR}
jar cmf ${SOURCE_MAIN_DIR}/manifest.mf ${BIN_DIR}/AIInterface.jar ${JAVA_PKG_FIRST_PART}
jar cf ${BIN_DIR}/AIInterface-src.jar -C "${SOURCE_MAIN_DIR}" ${JAVA_PKG_FIRST_PART}
jar uf ${BIN_DIR}/AIInterface-src.jar -C "${SOURCE_GENERATED_DIR}" ${JAVA_PKG_FIRST_PART}
#rm -Rf ${BUILD_DIR}
echo "	done."

cd ${CWD_BACKUP}


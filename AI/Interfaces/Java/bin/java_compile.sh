#!/bin/sh
#
# Compiles and packages the Java sources
#

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
cd ${THIS_DIR}

JAVA_PKG_FIRST_PART=com
JAVA_PKG=${JAVA_PKG_FIRST_PART}/clan_sy/spring/ai
JAVA_SRC_FILES="${JAVA_PKG}/*.java ${JAVA_PKG}/command/*.java ${JAVA_PKG}/event/*.java ${JAVA_PKG}/oo/*.java"
JLIB_DIR="../../data/jlib"

# !! Do not change from here on !!

cd ..

echo "	compiling ..."
mkdir -p build
cd ./java/src
javac -cp ".:${JLIB_DIR}/jna/jna.jar:${JLIB_DIR}/vecmath.jar" -d "../../build" ${JAVA_SRC_FILES}

echo "	packaging ..."
cd ../../build
jar cmf ../java/src/manifest.mf interface.jar ${JAVA_PKG_FIRST_PART}
#rm -R ./${JAVA_PKG_FIRST_PART}
jar cf interface-src.jar -C "../java/src" ${JAVA_PKG_FIRST_PART}
echo "	done."

cd ../bin

cd ${CWD_BACKUP}


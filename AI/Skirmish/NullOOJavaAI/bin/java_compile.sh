#!/bin/sh
#
# Compiles and packages the Java sources
#

JAVA_PKG_FIRST_PART=nulloojavaai
JAVA_PKG="${JAVA_PKG_FIRST_PART}"
JAVA_SRC_FILES="${JAVA_PKG}/*.java"

JAVA_INTERFACE_HOME="../../Interfaces/Java"
JAVA_INTERFACE_CP="${JAVA_INTERFACE_HOME}/data/jlib/jna/jna.jar:${JAVA_INTERFACE_HOME}/data/jlib/vecmath.jar:${JAVA_INTERFACE_HOME}/interface.jar"

# !! Do not change from here on !!

cd ..

echo "	compiling ..."
mkdir -p build
javac -cp ".:${JAVA_INTERFACE_CP}" -d build ${JAVA_SRC_FILES}

echo "	packaging ..."
jar cmf manifest.mf ai.jar -C build ${JAVA_PKG_FIRST_PART}
jar cf ai-src.jar ${JAVA_SRC_FILES}
mv ai*.jar build
#rm -R ./build/${JAVA_PKG_FIRST_PART}

echo "	done."
cd bin

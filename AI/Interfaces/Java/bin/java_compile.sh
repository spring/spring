#!/bin/sh
#
# Compiles the Java sources
#

JAVA_PKG=com/clan_sy/spring/ai
JAVA_PKG_J=com.clan_sy.spring.ai

echo "	compiling ..."
cd ../java/src
javac -cp "../../jlib/jna.jar:../../jlib/vecmath.jar" ${JAVA_PKG}/*.java ${JAVA_PKG}/command/*.java ${JAVA_PKG}/event/*.java ${JAVA_PKG}/oo/*.java
jar cmf manifest.mf interface.jar com
rm ${JAVA_PKG}/*.class ${JAVA_PKG}/command/*.class ${JAVA_PKG}/event/*.class ${JAVA_PKG}/oo/*.class
mv interface.jar ../..
cd ../../bin


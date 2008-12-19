@ECHO OFF
REM
REM Compiles and packages the Java sources
REM

JAVA_PKG_FIRST_PART=nulloojavaai
JAVA_PKG="%JAVA_PKG_FIRST_PART%"
SET JAVA_SRC_FILES="%JAVA_PKG%/*.java"

SET JAVA_INTERFACE_HOME="../../Interfaces/Java"
SET JAVA_INTERFACE_CP="%JAVA_INTERFACE_HOME%/data/jlib/jna/jna.jar;%JAVA_INTERFACE_HOME%/data/jlib/vecmath.jar;%JAVA_INTERFACE_HOME%/interface.jar"

REM !! Do not change from here on !!

cd ..

echo "	compiling ..."
mkdir build
javac -cp ".;%JAVA_INTERFACE_CP%" -d build %JAVA_SRC_FILES%

echo "	packaging ..."
jar cmf manifest.mf ai.jar -C build %JAVA_PKG_FIRST_PART%
jar cf ai-src.jar %JAVA_SRC_FILES%
mv ai*.jar build
#rmdir ./build/%JAVA_PKG_FIRST_PART%

echo "	done."
cd bin

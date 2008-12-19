@ECHO OFF
REM
REM Compiles and packages the Java sources
REM

SET JAVA_PKG_FIRST_PART=com
SET JAVA_PKG=%JAVA_PKG_FIRST_PART%/clan_sy/spring/ai
SET JAVA_SRC_FILES=%JAVA_PKG%/*.java %JAVA_PKG%/command/*.java %JAVA_PKG%/event/*.java %JAVA_PKG%/oo/*.java
SET JLIB_DIR="../../data/jlib"

# !! Do not change from here on !!

cd ..

echo "	compiling ..."
mkdir build
cd ./java/src
javac -cp "%JLIB_DIR%/jna.jar;%JLIB_DIR%/vecmath.jar" -d "../../build" %JAVA_SRC_FILES%

echo "	packaging ..."
cd ../../build
jar cmf manifest.mf interface.jar %JAVA_PKG_FIRST_PART%
REM rmdir ./%JAVA_PKG_FIRST_PART%
jar cf interface-src.jar -C "../java/src" %JAVA_PKG_FIRST_PART%

cd ../bin

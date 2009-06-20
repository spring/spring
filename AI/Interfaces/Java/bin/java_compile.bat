@ECHO OFF
REM
REM Compiles and packages the Java sources
REM

SET JAVA_PKG_FIRST_PART=com
SET JAVA_PKG=%JAVA_PKG_FIRST_PART%/springrts/ai

##############################################
### do not change anything below this line ###

SET JAVA_SRC_FILES=%JAVA_PKG%/*.java %JAVA_PKG%/command/*.java %JAVA_PKG%/event/*.java %JAVA_PKG%/oo/*.java

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
cd ..

echo "	compiling ..."
IF NOT EXIST build mkdir build
cd ./java/src
SET JLIB_DIR="../../data/jlib"
javac -cp "%JLIB_DIR%/jna.jar;%JLIB_DIR%/vecmath.jar" -d "../../build" %JAVA_SRC_FILES%

echo "	packaging ..."
cd ../../build
jar cmf ../java/src/manifest.mf interface.jar %JAVA_PKG_FIRST_PART%
jar cf interface-src.jar -C "../java/src" %JAVA_PKG_FIRST_PART%
mv *.jar ..
cd ..
rmdir ./build

cd bin


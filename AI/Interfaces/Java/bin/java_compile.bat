@ECHO OFF
REM
REM Compiles the Java sources
REM

SET JAVA_PKG=com/clan_sy/spring/ai
SET JAVA_PKG_J=com.clan_sy.spring.ai

ECHO "	compiling ..."
cd ../java/src
javac -cp "../../jlib/jna.jar;../../jlib/vecmath.jar" %JAVA_PKG%/*.java %JAVA_PKG%/command/*.java %JAVA_PKG%/event/*.java  %JAVA_PKG%/oo/*.java
jar cmf manifest.mf interface.jar com
rm %JAVA_PKG%/*.class %JAVA_PKG%/command/*.class %JAVA_PKG%/event/*.class  %JAVA_PKG%/oo/*.class
mv interface.jar ../..
cd ../../bin


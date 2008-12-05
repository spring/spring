@ECHO OFF
REM
REM Move the Java Part of the interface to its final destination.
REM @param	interface version (eg: "0.1")
REM

SET INTERFACE_VERSION=%1
IF "%INTERFACE_VERSION%"=="" (
	ECHO "Usage %0 [java-interface-version]"
	EXIT 1
)

SET MY_HOME=..
SET SPRING_HOME=%MY_HOME%/../../..

ECHO "	installing ..."
mkdir -p %SPRING_HOME%/game/AI/Interfaces/Java/%INTERFACE_VERSION%/jlib
cp %MY_HOME%/interface.jar %SPRING_HOME%/game/AI/Interfaces/Java/%INTERFACE_VERSION%


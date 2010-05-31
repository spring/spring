@ECHO OFF
REM
REM Move the Java Part of the interface to its final destination.
REM @param	interface version (eg: "0.1")
REM

SET INTERFACE_VERSION=%1
SET SPRING_DATA_DIR=%2
IF "%INTERFACE_VERSION%"=="" IF "%SPRING_DATA_DIR%"=="" (
	ECHO "Usage %0 [java-interface-version] [spring-data-dir]"
	EXIT 1
)

SET HOME_DIR=..

##############################################
### do not change anything below this line ###

ECHO "	installing ..."
mkdir %SPRING_DATA_DIR%/AI/Interfaces/Java/%INTERFACE_VERSION%/jlib
cp %HOME_DIR%/interface.jar %SPRING_DATA_DIR%/AI/Interfaces/Java/%INTERFACE_VERSION%
cp %HOME_DIR%/interface-src.jar %SPRING_DATA_DIR%/AI/Interfaces/Java/%INTERFACE_VERSION%


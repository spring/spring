@ECHO OFF
REM
REM Go through the whole Java build process
REM @param_1	interface version (eg: "0.1")
REM

SET INTERFACE_VERSION=%1
IF "%INTERFACE_VERSION%"=="" (
	ECHO "Usage %0 [java-interface-version]"
	EXIT 1
)

call ./java_generateWrappers.bat
call ./java_compile.bat
call ./java_install.bat %INTERFACE_VERSION%


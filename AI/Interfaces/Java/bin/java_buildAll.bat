@ECHO OFF
REM
REM Go through the whole Java build process
REM @param_1	interface version (eg: "0.1")
REM

IF {%1}=={} IF {%2}=={} (
	ECHO "Usage %0 [java-interface-version] [spring-data-dir]"
	EXIT 1
)
SET INTERFACE_VERSION=%1
SET SPRING_DATA_DIR=%2

##############################################
### do not change anything below this line ###

call ./java_generateWrappers.bat
call ./java_compile.bat
call ./java_install.bat %INTERFACE_VERSION% %SPRING_DATA_DIR%


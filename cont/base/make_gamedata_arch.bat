@echo off
setlocal

rem This will build the following, needed to run spring:
rem * %BUILD_DIR%\spring\bitmaps.sdz
rem * %BUILD_DIR%\springcontent.sdz
rem * %BUILD_DIR%\maphelper.sdz
rem * %BUILD_DIR%\cursors.sdz

rem absolute or relative to spring source root
set BUILD_DIR=%1
IF "%BUILD_DIR%" == "" (
	rem default:
	set BUILD_DIR=build\base\
)

rem Move to spring source root
cd %~dp0..\..

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

rem make BUILD_DIR absolute, if it is not yet
for %%a in (cd) do set BUILD_DIR=%%~dpa

cd %~dp0


rem Interpret second cmd-line param as path to 7-Zip executable
set EXEC_7Z=%2

rem Look for 7za.exe in CWD
IF "%EXEC_7Z%" == "" (
	set EXEC_7Z=%~dp07za.exe
)
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z

rem Look for 7za.exe in MINGWLIBS
set EXEC_7Z=%MINGWLIBS%\bin\7za.exe
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z

rem Look for 7za.exe in VCLIBS
set EXEC_7Z=%VCLIBS%\bin\7za.exe
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z

rem Look for 7za.exe in PATH
set EXEC_7Z=7za.exe
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z

rem Look for 7z.exe in PATH
set EXEC_7Z=7z.exe
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z

rem Give up trying to find 7za.exe
echo 7zip executable (7za.exe or 7z.exe) not found, please make sure it is in your PATH environment variable.
exit /B 1

rem Found 7za.exe
:ok7z
set CMD_7Z=%EXEC_7Z% u -tzip -r


rem Only copy to a temp for converting line endings if
rem the git config value core.autocrlf is set to false,
rem because only in this case line endings will be in
rem the windows format, and therefore have to be converted.
rem You may enforce this by setting the following in the
rem environment this script is executed in:
rem USE_TMP_DIR=TRUE
FOR /F "usebackq" %%P IN (`git config --get core.autocrlf`) DO if "%%P" == "false" set USE_TMP_DIR=TRUE


if not "%USE_TMP_DIR%" == "TRUE" goto tmpDir_no
:tmpDir_yes

	rem Interpret 3rd cmd-line param as path to dos2unix executable
	set EXEC_D2U=%3

	set TEST_TEXT_FILE=%BUILD_DIR%\tmp.txt
	echo bla > %TEST_TEXT_FILE%

	rem Look for dos2unix.exe in CWD
	IF "%EXEC_D2U%" == "" (
		set EXEC_D2U=%~dp0dos2unix.exe
	)
	%EXEC_D2U% %TEST_TEXT_FILE% > NUL 2>&1
	if "%ERRORLEVEL%" == "0" goto okD2U

	rem Look for dos2unix.exe in MINGWLIBS
	set EXEC_D2U=%MINGWLIBS%\bin\dos2unix.exe
	%EXEC_D2U% %TEST_TEXT_FILE% > NUL 2>&1
	if "%ERRORLEVEL%" == "0" goto okD2U

	rem Look for dos2unix.exe in VCLIBS
	set EXEC_D2U=%VCLIBS%\bin\dos2unix.exe
	%EXEC_D2U% %TEST_TEXT_FILE% > NUL 2>&1
	if "%ERRORLEVEL%" == "0" goto okD2U

	rem Look for dos2unix.exe in PATH
	set EXEC_D2U=dos2unix.exe
	%EXEC_D2U% %TEST_TEXT_FILE% > NUL 2>&1
	if "%ERRORLEVEL%" == "0" goto okD2U

	rem Give up trying to find dos2unix.exe
	echo dos2unix.exe not found, please make sure it is in your PATH environment variable.
	del /F %TEST_TEXT_FILE%
	exit /B 1
:okD2U
	del /F %TEST_TEXT_FILE%

	set TMP_DIR=%BUILD_DIR%_tmp
	echo Copying to temporary directory ...
	rem rmdir /S /Q %TMP_DIR%
	mkdir %TMP_DIR% > NUL 2>&1
	xcopy /E /I /Q /H /Y * %TMP_DIR% > NUL 2>&1
	cd %TMP_DIR%
	rem Convert line endings to unix format to not desync
	rem with linux assembled base files.
	rem This is only needed cause of quirks in git's line endings
	rem handling on windows.
	echo Converting line endings to unix format ...
	rem This for takes long, but only for the batch part of it,
	rem not the actual converting
	FOR /F "usebackq" %%F IN (`dir /b /s *`) DO (call :s_convLineEnds "%%F")
	goto tmpDir_end
:tmpDir_no
	echo Caution: Not converting line endings of base files.
	echo          In case you experience desyncs, use build files
	echo          from the buildbot.
	goto tmpDir_end

rem Batch subroutine for converting a files line endings
rem to unix style, if it is a text file.
:s_convLineEnds
	if "%~x1" == ".txt" goto isTxtFile
	if "%~x1" == ".lua" goto isTxtFile
	if "%~x1" == ".glsl" goto isTxtFile
	if "%~x1" == ".fp" goto isTxtFile
	if "%~x1" == ".vp" goto isTxtFile
	goto isBinFile

	:isTxtFile
	%EXEC_D2U% %1 > NUL
	goto s_convLineEnds_end

	:isBinFile
	goto s_convLineEnds_end

	:s_convLineEnds_end
	goto :eof

:tmpDir_end


rem make sure the destination exists
if not exist "%BUILD_DIR%\spring" mkdir "%BUILD_DIR%\spring"

echo Updating bitmaps.sdz
if exist "%BUILD_DIR%\spring\bitmaps.sdz" del "%BUILD_DIR%\spring\bitmaps.sdz"
cd bitmaps
%CMD_7Z% %BUILD_DIR%\spring\bitmaps.sdz * > NUL
cd ..

echo Updating springcontent.sdz
if exist "%BUILD_DIR%\springcontent.sdz" del "%BUILD_DIR%\springcontent.sdz"
cd springcontent
%CMD_7Z% %BUILD_DIR%\springcontent.sdz * > NUL
cd ..

echo Updating maphelper.sdz
if exist "%BUILD_DIR%\maphelper.sdz" del "%BUILD_DIR%\maphelper.sdz"
cd maphelper
%CMD_7Z% %BUILD_DIR%\maphelper.sdz * > NUL
cd ..

echo Updating cursors.sdz
if exist "%BUILD_DIR%\cursors.sdz" del "%BUILD_DIR%\cursors.sdz"
cd cursors
%CMD_7Z% %BUILD_DIR%\cursors.sdz * > NUL
cd ..

if not "%USE_TMP_DIR%" == "TRUE" goto theEnd
cd ..
rmdir /S /Q %TMP_DIR%

:theEnd
endlocal


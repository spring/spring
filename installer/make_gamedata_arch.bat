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
	set BUILD_DIR=game\base\
)

cd %~dp0..
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
rem make BUILD_DIR absolute, if it is not yet
for %%a in (cd) do set BUILD_DIR=%%~dpa

rem move to spring source root
cd %~dp0..

set EXEC_7Z=%~dp07za.exe
rem set EXEC_7Z=7z
%EXEC_7Z% > NUL 2>&1
if "%ERRORLEVEL%" == "0" goto ok7z
echo %EXEC_7Z% not found, please make sure it is in your PATH environment variable.
exit /B 1
:ok7z
set CMD_7Z=%EXEC_7Z% u -tzip -r -x!README.txt

cd %~dp0builddata

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

cd ..
endlocal


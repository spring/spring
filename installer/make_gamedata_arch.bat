@echo off
setlocal

rem This will build the following, needed to run spring:
rem * %BUILD_DIR%\spring\bitmaps.sdz
rem * %BUILD_DIR%\springcontent.sdz
rem * %BUILD_DIR%\maphelper.sdz
rem * %BUILD_DIR%\cursors.sdz

rem absolute or relative to spring source root
set BUILD_DIR="%1"
IF %BUILD_DIR%=="" (
	rem default:
	set BUILD_DIR="game\base\"
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

cd %~dp0\builddata

rem make sure the destination exists
if not exist "%BUILD_DIR%\spring" mkdir "%BUILD_DIR%\spring"

echo Creating bitmaps.sdz
if exist "%BUILD_DIR%\spring\bitmaps.sdz" del "%BUILD_DIR%\spring\bitmaps.sdz"
cd bitmaps
%EXEC_7Z% a -tzip -r -x!README.txt %BUILD_DIR%\spring\bitmaps.sdz *
cd ..

echo Creating springcontent.sdz
if exist "%BUILD_DIR%\springcontent.sdz" del "%BUILD_DIR%\springcontent.sdz"
cd springcontent
%EXEC_7Z% a -tzip %BUILD_DIR%\springcontent.sdz *
cd ..

echo Creating maphelper.sdz
if exist "%BUILD_DIR%\maphelper.sdz" del "%BUILD_DIR%\maphelper.sdz"
cd maphelper
%EXEC_7Z% a -tzip %BUILD_DIR%\maphelper.sdz *
cd ..

echo Creating cursors.sdz
if exist "%BUILD_DIR%\cursors.sdz" del "%BUILD_DIR%\cursors.sdz"
cd cursors
%EXEC_7Z% a -tzip %BUILD_DIR%\cursors.sdz *
cd ..

cd ..
endlocal

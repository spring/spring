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
	# default:
	set BUILD_DIR="game\base\"
)

cd %~dp0..
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
rem make BUILD_DIR absolute, if it is not yet
for %%a in (cd) do set BUILD_DIR=%%~dpa

# move to spring source root
cd %~dp0..

cd builddata

rem make sure the destination exists
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\spring mkdir %BUILD_DIR%\spring

echo Creating bitmaps.sdz
del /Q %BUILD_DIR%\spring\bitmaps.sdz
cd bitmaps
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
cd ..\..
pkzip -add _temp.zip builddata\bitmaps\modinfo.lua
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz %BUILD_DIR%\spring
cd builddata

echo Creating springcontent.sdz
del /Q %BUILD_DIR%\springcontent.sdz
cd springcontent
..\..\pkzip -add -dir=current ..\..\_temp.zip shaders\*
..\..\pkzip -add -dir=current ..\..\_temp.zip gamedata\*
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
..\..\pkzip -add -dir=current ..\..\_temp.zip anims\*
..\..\pkzip -add -dir=current ..\..\_temp.zip LuaGadgets\*
cd ..\..
pkzip -add _temp.zip builddata\springcontent\modinfo.lua
rename _temp.zip springcontent.sdz
move /Y springcontent.sdz %BUILD_DIR%
cd builddata

echo Creating maphelper.sdz
del /Q %BUILD_DIR%\maphelper.sdz
cd maphelper
..\..\pkzip -add -dir=current ..\..\_temp.zip maphelper\*
cd ..\..
pkzip -add _temp.zip builddata\maphelper\modinfo.lua
pkzip -add _temp.zip builddata\maphelper\MapOptions.lua
rename _temp.zip maphelper.sdz
move /Y maphelper.sdz %BUILD_DIR%
cd builddata

echo Creating cursors.sdz
del /Q %BUILD_DIR%\cursors.sdz
cd cursors
..\..\pkzip -add -dir=current ..\..\_temp.zip anims\*
cd ..\..
pkzip -add _temp.zip builddata\cursors\modinfo.lua
rename _temp.zip cursors.sdz
move /Y cursors.sdz %BUILD_DIR%
cd builddata

cd ..
endlocal

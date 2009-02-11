@echo off
echo This will build game\base\springcontent.sdz and
echo game\base\spring\bitmaps.sdz needed to run spring

rem make sure the destination exists
if not exist ..\..\game\base mkdir ..\..\game\base
if not exist ..\..\game\base\spring mkdir ..\..\game\base\spring

cd builddata

echo Creating bitmaps.sdz
del /Q ..\..\game\base\spring\bitmaps.sdz
cd bitmaps
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
cd ..\..
pkzip -add _temp.zip builddata\bitmaps\modinfo.lua
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz ..\game\base\spring
cd builddata

echo Creating springcontent.sdz
del /Q ..\..\game\base\springcontent.sdz
cd springcontent
..\..\pkzip -add -dir=current ..\..\_temp.zip shaders\*
..\..\pkzip -add -dir=current ..\..\_temp.zip gamedata\*
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
..\..\pkzip -add -dir=current ..\..\_temp.zip anims\*
..\..\pkzip -add -dir=current ..\..\_temp.zip LuaGadgets\*
cd ..\..
pkzip -add _temp.zip builddata\springcontent\modinfo.lua
rename _temp.zip springcontent.sdz
move /Y springcontent.sdz ..\game\base
cd builddata

echo Creating maphelper.sdz
del /Q ..\..\game\base\maphelper.sdz
cd maphelper
..\..\pkzip -add -dir=current ..\..\_temp.zip maphelper\*
cd ..\..
pkzip -add _temp.zip builddata\maphelper\modinfo.lua
pkzip -add _temp.zip builddata\maphelper\MapOptions.lua
rename _temp.zip maphelper.sdz
move /Y maphelper.sdz ..\game\base
cd builddata

echo Creating cursors.sdz
del /Q ..\..\game\base\cursors.sdz
cd cursors
..\..\pkzip -add -dir=current ..\..\_temp.zip anims\*
cd ..\..
pkzip -add _temp.zip builddata\cursors\modinfo.lua
rename _temp.zip cursors.sdz
move /Y cursors.sdz ..\game\base
cd builddata

cd ..

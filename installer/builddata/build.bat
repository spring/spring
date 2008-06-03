@echo off
echo This will build game\base\springcontent.sdz and
echo game\base\spring\bitmaps.sdz needed to run spring

echo Creating bitmaps.sdz
del /Q ..\..\base\spring\bitmaps.sdz
cd bitmaps
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
cd ..\..
pkzip -add _temp.zip builddata\bitmaps\modinfo.tdf
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz ..\game\base\spring
cd builddata

echo Creating springcontent.sdz
del /Q ..\..\base\springcontent.sdz
cd springcontent
..\..\pkzip -add -dir=current ..\..\_temp.zip shaders\*
..\..\pkzip -add -dir=current ..\..\_temp.zip gamedata\*
..\..\pkzip -add -dir=current ..\..\_temp.zip bitmaps\*
..\..\pkzip -add -dir=current ..\..\_temp.zip anims\*
..\..\pkzip -add -dir=current ..\..\_temp.zip LuaGadgets\*
cd ..\..
pkzip -add _temp.zip builddata\springcontent\modinfo.tdf
rename _temp.zip springcontent.sdz
move /Y springcontent.sdz ..\game\base
cd builddata

echo Creating maphelper.sdz
del /Q ..\..\base\maphelper.sdz
cd maphelper
..\..\pkzip -add -dir=current ..\..\_temp.zip maphelper\*
cd ..\..
pkzip -add _temp.zip builddata\maphelper\modinfo.tdf
pkzip -add _temp.zip builddata\maphelper\MapOptions.lua
rename _temp.zip maphelper.sdz
move /Y maphelper.sdz ..\game\base
cd builddata

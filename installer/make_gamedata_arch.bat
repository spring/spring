@echo off
echo Creating bitmaps.sdz
del /Q ..\base\spring\bitmaps.sdz
cd ..\game
..\installer\pkzip -add -dir=current ..\installer\_temp.zip bitmaps\*
cd ..\installer
pkzip -add _temp.zip builddata\bitmaps\modinfo.tdf
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz ..\game\base\spring

echo Creating springcontent.sdz
del /Q ..\base\springcontent.sdz
cd ..\game
..\installer\pkzip -add -dir=current ..\installer\_temp.zip gamedata\*
cd ..\installer
pkzip -add _temp.zip builddata\springcontent\modinfo.tdf
rename _temp.zip springcontent.sdz
move /Y springcontent.sdz ..\game\base

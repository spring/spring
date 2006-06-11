echo Creating bitmaps.sdz
del /Q ..\base\bitmaps.sdz
cd ..\game
..\installer\pkzip -add -dir=current ..\installer\_temp.zip bitmaps\*
cd ..\installer
pkzip -add _temp.zip builddata\modinfo.tdf
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz ..\game\base\spring


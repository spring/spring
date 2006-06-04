@echo off
cls
echo This will create the various installers for a TA Spring release
echo .
echo Remember to generate all needed path information for the maps to be included
echo Press any key to continue..
echo .
echo Also remember to update the version string in GameVersion.h so that
echo the correct .pdb can be identified!
echo.
pause > nul

echo Creating bitmaps.sdz
del /Q ..\base\bitmaps.sdz
cd ..\game
..\installer\pkzip -add -dir=current ..\installer\_temp.zip bitmaps\*
cd ..\installer
pkzip -add _temp.zip builddata\modinfo.tdf
rename _temp.zip bitmaps.sdz
move /Y bitmaps.sdz ..\game\base\spring

echo Creating test installer
"C:\Program Files\NSIS\makensis.exe" /V3 /DTEST_BUILD taspring.nsi 

echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..
echo Press any key to continue..
pause > nul

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

call build_code

echo Creating test installer
"C:\Program Files\NSIS\makensis.exe" /V3 /DTEST_BUILD spring.nsi 

echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..
echo Press any key to continue..
pause > nul

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

echo Creating normal installer
"C:\Program Files\NSIS\makensis.exe" /V3 /DMINGW spring.nsi 

echo Creating GPL installer
"C:\Program Files\NSIS\makensis.exe" /V3 /DMINGW /DNO_TOTALA spring.nsi 

echo Creating update installer
"C:\Program Files\NSIS\makensis.exe" /V3 /DMINGW /DSP_UPDATE spring.nsi  

rem echo Creating patch installer
rem "C:\Program Files\NSIS\makensis.exe" /V3 /DSP_PATCH spring.nsi

rem echo Creating installer without cavedog content
rem "C:\Program Files\NSIS\makensis.exe" /V3 /DSP_ALLMAPS spring.nsi 

rem echo Creating small installer without cavedog content
rem "C:\Program Files\NSIS\makensis.exe" /V3 spring.nsi 

rem echo Creating core only installer
rem "C:\Program Files\NSIS\makensis.exe" /V3 /DSP_CORE spring.nsi 

echo All done.. 
echo If this is a public release, make sure to save this and tag CVS etc..
echo Press any key to continue..
pause > nul

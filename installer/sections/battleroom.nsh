
!ifdef INSTALL
  SetOutPath "$INSTDIR"

  ; The battleroom
;  File "..\game\ClientControls.dll"
  File "..\game\TASClient.exe"
;  File "..\game\SpringClient.pdb"
;  File "..\game\Utility.dll"
  File "..\game\Unitsync.dll"
  
  CreateDirectory "$INSTDIR\lobby\cache"
  CreateDirectory "$INSTDIR\lobby\var"
  CreateDirectory "$INSTDIR\lobby\logs"
  
;  SetOutPath "$INSTDIR\lobby\sidepics"
;  File "..\game\lobby\sidepics\arm.bmp"
;  File "..\game\lobby\sidepics\core.bmp"
;  File "..\game\lobby\sidepics\tll.bmp"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"
  RmDir "$INSTDIR\lobby\sidepics"
!else

  ; The battleroom
  Delete "$INSTDIR\TASClient.exe"
  Delete "$INSTDIR\Unitsync.dll"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"

!endif

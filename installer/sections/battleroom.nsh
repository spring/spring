!ifdef INSTALL
  SetOutPath "$INSTDIR"

  ; The battleroom
  File "..\game\TASClient.exe"
  File "..\game\unitsync.dll"

  CreateDirectory "$INSTDIR\lobby\cache"
  CreateDirectory "$INSTDIR\lobby\cache\online"
  CreateDirectory "$INSTDIR\lobby\cache\maps"
  CreateDirectory "$INSTDIR\lobby\var"
  CreateDirectory "$INSTDIR\lobby\logs"

!else

  ; The battleroom
  Delete "$INSTDIR\TASClient.exe"
  Delete "$INSTDIR\unitsync.dll"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"

!endif ; !INSTALL

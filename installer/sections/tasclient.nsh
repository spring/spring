!ifdef INSTALL
  SetOutPath "$INSTDIR"

  ; The battleroom
  File "..\game\TASClient.exe"
  File "..\game\7za.dll"
  
  CreateDirectory "$INSTDIR\lobby\cache"
  CreateDirectory "$INSTDIR\lobby\cache\online"
  CreateDirectory "$INSTDIR\lobby\cache\maps"
  CreateDirectory "$INSTDIR\lobby\var"
  CreateDirectory "$INSTDIR\lobby\logs"

!else

  ; The battleroom
  Delete "$INSTDIR\TASClient.exe"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"
  RmDir "$INSTDIR\lobby\cache\maps"
  RmDir "$INSTDIR\lobby\cache\online"
  RmDir "$INSTDIR\lobby\cache"
  RmDir "$INSTDIR\lobby\logs"
  RmDir "$INSTDIR\lobby\var"
  RmDir "$INSTDIR\lobby"

!endif ; !INSTALL

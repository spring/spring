!ifdef INSTALL
  SetOutPath "$INSTDIR"

  ; The battleroom
  File "..\external\TASClient.exe"
  File "..\external\7za.dll"
  
  
  CreateDirectory "$INSTDIR\lobby\cache"
  CreateDirectory "$INSTDIR\lobby\cache\online"
  CreateDirectory "$INSTDIR\lobby\cache\maps"
  CreateDirectory "$INSTDIR\lobby\cache\mods"
  CreateDirectory "$INSTDIR\lobby\var"
  CreateDirectory "$INSTDIR\lobby\logs"

  SetOutPath "$INSTDIR\lobby\var"
  File "..\Lobby\TASClient\lobby\var\groups.ini"
!else

  ; The battleroom
  Delete "$INSTDIR\TASClient.exe"
  Delete "$INSTDIR\7za.dll"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"
  Delete "$INSTDIR\lobby\var\groups.ini"
  RmDir "$INSTDIR\lobby\cache\maps"
  RmDir "$INSTDIR\lobby\cache\mods"
  RmDir "$INSTDIR\lobby\cache\online"
  RmDir "$INSTDIR\lobby\cache"
  RmDir "$INSTDIR\lobby\logs"
  RmDir "$INSTDIR\lobby\var"
  RmDir "$INSTDIR\lobby"

!endif ; !INSTALL

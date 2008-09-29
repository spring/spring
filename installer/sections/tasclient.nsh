!ifdef INSTALL
  SetOutPath "$INSTDIR"

  ; The battleroom
  File "..\external\TASClient.exe"
  File "..\external\7za.dll"

  inetc::get \
             "http://installer.clan-sy.com/SpringDownloader.exe" "$INSTDIR\SpringDownloader.exe"          
  
  
  CreateDirectory "$INSTDIR\lobby\cache"
  CreateDirectory "$INSTDIR\lobby\cache\online"
  CreateDirectory "$INSTDIR\lobby\cache\maps"
  CreateDirectory "$INSTDIR\lobby\cache\mods"
  CreateDirectory "$INSTDIR\lobby\var"
  CreateDirectory "$INSTDIR\lobby\var\replayFilters"
  CreateDirectory "$INSTDIR\lobby\logs"
  CreateDirectory "$INSTDIR\lobby\python\"

  SetOutPath "$INSTDIR\lobby\var"
  File "..\Lobby\TASClient\lobby\var\groups.ini"
  File "..\Lobby\TASClient\lobby\var\tips.txt"

  Call GetDotNETVersion
  Pop $0
  StrCpy $0 $0 "" 1 ; Remove the starting "v" so $0 contains only the version number.
  ${VersionCompare} $0 "2.0" $1
  ${If} $0 == "ot found" ; not a typo
    Call NoDotNet
  ${ElseIf} $1 == 2
    Call OldDotNet
  ${EndIf}

!else

  ; The battleroom
  ExecWait "$INSTDIR\SpringDownloader.exe -uninstall"
  Delete "$INSTDIR\TASClient.exe"
  Delete "$INSTDIR\7za.dll"
  Delete "$INSTDIR\lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\lobby\sidepics\core.bmp"
  Delete "$INSTDIR\lobby\sidepics\tll.bmp"
  Delete "$INSTDIR\lobby\var\groups.ini"
  Delete "$INSTDIR\lobby\var\tips.txt"
  RmDir "$INSTDIR\lobby\cache\maps"
  RmDir "$INSTDIR\lobby\cache\mods"
  RmDir "$INSTDIR\lobby\cache\online"
  RmDir "$INSTDIR\lobby\cache"
  RmDir "$INSTDIR\lobby\logs"
  RmDir "$INSTDIR\lobby\python\"
  RmDir "$INSTDIR\lobby\var\replayFilters"
  RmDir "$INSTDIR\lobby\var"
  RmDir "$INSTDIR\lobby"
  Delete "$INSTDIR\SpringDownloader.exe"

!endif ; !INSTALL

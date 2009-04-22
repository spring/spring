!ifdef INSTALL
  SetOutPath "$INSTDIR"

  CreateDirectory "$INSTDIR\Lobby\cache"
  CreateDirectory "$INSTDIR\Lobby\cache\online"
  CreateDirectory "$INSTDIR\Lobby\cache\maps"
  CreateDirectory "$INSTDIR\Lobby\cache\mods"
  CreateDirectory "$INSTDIR\Lobby\var"
  CreateDirectory "$INSTDIR\Lobby\var\replayFilters"
  CreateDirectory "$INSTDIR\Lobby\logs"
  CreateDirectory "$INSTDIR\Lobby\python"

  ; The battleroom with all files (script needs to remove groups.ini)
  File /r "..\installer\TASClient\SaveFiles\*.*"

  inetc::get \
             "http://files.caspring.org/caupdater/SpringDownloader.exe" "$INSTDIR\SpringDownloader.exe"
  inetc::get \
             "http://springrts.com/dl/TASClient.exe" "$INSTDIR\TASClient.exe"

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
  Delete "$INSTDIR\Lobby\sidepics\arm.bmp"
  Delete "$INSTDIR\Lobby\sidepics\core.bmp"
  Delete "$INSTDIR\Lobby\sidepics\tll.bmp"
  Delete "$INSTDIR\Lobby\var\groups.ini"
  Delete "$INSTDIR\Lobby\var\tips.txt"
  RmDir "$INSTDIR\Lobby\cache\maps"
  RmDir "$INSTDIR\Lobby\cache\mods"
  RmDir "$INSTDIR\Lobby\cache\online"
  RmDir "$INSTDIR\Lobby\cache"
  RmDir "$INSTDIR\Lobby\logs"
  RmDir "$INSTDIR\Lobby\python"
  RmDir "$INSTDIR\Lobby\var\replayFilters"
  RmDir "$INSTDIR\Lobby\var"
  RmDir "$INSTDIR\Lobby"
  Delete "$INSTDIR\SpringDownloader.exe"

!endif ; !INSTALL

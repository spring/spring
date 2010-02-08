!ifdef INSTALL
  SetOutPath "$INSTDIR"

  inetc::get "http://files.caspring.org/caupdater/SpringDownloader.exe" "$INSTDIR\SpringDownloader.exe"

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

  ExecWait "$INSTDIR\SpringDownloader.exe -uninstall"
  Delete "$INSTDIR\SpringDownloader.exe"

!endif ; !INSTALL

!ifdef INSTALL
  StrCpy $CA "true"
  SetOutPath "$INSTDIR"
  
;  inetc::get \
             "http://installer.clan-sy.com/CaDownloader.exe" "$INSTDIR\CaDownloader.exe" \         

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
  ExecWait  "$INSTDIR\CaDownloader.exe -uninstall"
  Delete "$INSTDIR\CaDownloader.exe"
  Delete "$INSTDIR\base\tatextures_v062.sdz"
  Delete "$INSTDIR\base\tacontent_v2.sdz"
  Delete "$INSTDIR\base\otacontent.sdz"
  RmDir "$INSTDIR\base"
  RmDir "$INSTDIR\mods"

!endif

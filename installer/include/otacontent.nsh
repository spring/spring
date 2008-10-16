;Three functions which check to make sure OTA Content is installed before installing mods that depend on it.

Function CheckTATextures
  ClearErrors
  FileOpen $0 "$INSTDIR\base\tatextures_v062.sdz" r
  IfErrors Fail
  FileSeek $0 0 END $1

  IntCmp $1 1245637 Done
Fail:
  inetc::get \
             "http://installer.clan-sy.com/base/tatextures_v062.sdz" "$INSTDIR\base\tatextures_v062.sdz"
Done:
  FileClose $0

FunctionEnd


Function CheckOTAContent
  ClearErrors
  FileOpen $0 "$INSTDIR\base\otacontent.sdz" r
  IfErrors Fail
  FileSeek $0 0 END $1

  IntCmp $1 7421640 Done
Fail:
  inetc::get \
             "http://installer.clan-sy.com/base/otacontent.sdz" "$INSTDIR\base\otacontent.sdz"
Done:
  FileClose $0

FunctionEnd


Function CheckTAContent
  ClearErrors
  FileOpen $0 "$INSTDIR\base\tacontent_v2.sdz" r
  IfErrors Fail
  FileSeek $0 0 END $1

  IntCmp $1 284 Done
Fail:
  inetc::get \
             "http://installer.clan-sy.com/base/tacontent_v2.sdz" "$INSTDIR\base\tacontent_v2.sdz"
Done:
  FileClose $0

FunctionEnd

!ifdef INSTALL

  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\RAI-0.601.dll"

!else

  Delete "$INSTDIR\AI\Bot-libs\RAI-0.601.dll"
  RmDir /r "$INSTDIR\AI\RAI"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Bot-libs"
  RmDir "$INSTDIR\AI"

!endif

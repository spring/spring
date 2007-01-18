!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\KAI-0.12.dll"
!else
  Delete "$INSTDIR\AI\Bot-libs\KAI-0.12.dll"
  RmDir /r "$INSTDIR\AI\KAI"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Bot-libs"
  RmDir "$INSTDIR\AI"
!endif

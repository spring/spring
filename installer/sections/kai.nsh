!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\KAIK-0.13.dll"
!else
  Delete "$INSTDIR\AI\Bot-libs\KAIK-0.13.dll"
  RmDir /r "$INSTDIR\AI\KAIK013"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Bot-libs"
  RmDir "$INSTDIR\AI"
!endif

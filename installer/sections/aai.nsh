!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\AAI.dll"

  SetOutPath "$INSTDIR\AI\AAI"
  File /r /x .svn "..\game\AI\AAI\*.*"
!else
  Delete "$INSTDIR\AI\Bot-libs\AAI.dll"
  RmDir /r "$INSTDIR\AI\AAI"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Bot-libs"
  RmDir "$INSTDIR\AI"
!endif

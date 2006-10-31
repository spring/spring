!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\AAI.dll"

  SetOutPath "$INSTDIR\AI\AAI"
  File /r /x .svn "..\game\AI\AAI\*.*"
!else
  Delete "$INSTDIR\AI\Bot-libs\AAI.dll"
  RMDir /r "$INSTDIR\AI\AAI"
!endif

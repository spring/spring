!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\aai.dll"

  SetOutPath "$INSTDIR\AI\aai"
  File /r /x .svn "..\game\AI\aai\*.*"
!else
  Delete "$INSTDIR\AI\Bot-libs\aai.dll"
  RMDir /r "$INSTDIR\AI\aai"
!endif

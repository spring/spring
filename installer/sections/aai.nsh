!ifdef INSTALL
  SetOutPath "$INSTDIR\aidll\globalai"
  File "..\game\aidll\globalai\aai.dll"

  SetOutPath "$INSTDIR\aidll\globalai\aai"
  File /r "..\game\aidll\globalai\aai\*.*"
!else
  Delete "$INSTDIR\aidll\globalai\aai.dll"
  RMDir /r "$INSTDIR\aidll\globalai\aai"
!endif

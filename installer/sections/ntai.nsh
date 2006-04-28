
!ifdef INSTALL
  SetOutPath "$INSTDIR\aidll\globalai"
  File "..\game\aidll\globalai\ntai.dll"
  CreateDirectory "..\game\aidll\globalai\MEXCACHE"
  SetOutPath "$INSTDIR\aidll\globalai\NTAI"
  File /x "svn" /r "..\game\aidll\globalai\ntai\*.*"
!else
  Delete "$INSTDIR\aidll\globalai\ntai.dll"
  Delete "$INSTDIR\aidll\globalai\mexcache\*.*"
  RMDir /r "$INSTDIR\aidll\globalai\ntai"
!endif


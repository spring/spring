
!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Bot-libs"
  File "..\game\AI\Bot-libs\ntai.dll"
  CreateDirectory "..\AI\NTAI\MEXCACHE"
  SetOutPath "$INSTDIR\AI\NTAI"
  File /r /x .svn "..\game\AI\NTAI\*.*"
!else
  Delete "$INSTDIR\AI\Bot-libs\ntai.dll"
  RMDir /r "$INSTDIR\AI\NTAI"
!endif


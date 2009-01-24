!ifdef INSTALL
  SetOutPath "$INSTDIR\docs"
  Delete "$INSTDIR\docs\readme.html"
  File "..\LICENSE.html"
  File "..\Documentation\changelog.txt"
  File "..\Documentation\cmds.txt"
  Delete "$INSTDIR\docs\xtachanges.txt"

  File "..\Documentation\userdocs\Q&A.html"
  File "..\Documentation\userdocs\Getting Started.html"
  File "..\Documentation\userdocs\Legal.html"
  File "..\Documentation\userdocs\main.html"
  File "..\Documentation\userdocs\More Info.html"
!else
  ; Documentation
  Delete "$INSTDIR\docs\readme.html"
  Delete "$INSTDIR\docs\LICENSE.html"
  Delete "$INSTDIR\docs\changelog.txt"
  Delete "$INSTDIR\docs\cmds.txt"
  Delete "$INSTDIR\docs\FAQ.html"
  Delete "$INSTDIR\docs\GettingStarted.html"
  Delete "$INSTDIR\docs\Legal.html"
  Delete "$INSTDIR\docs\main.html"
  Delete "$INSTDIR\docs\MoreInfo.html"
  RMDir "$INSTDIR\docs"
!endif

!ifdef INSTALL
	SetOutPath "$INSTDIR\docs"
	Delete "$INSTDIR\docs\readme.html"
	File "..\LICENSE.html"
	File "..\doc\changelog.txt"
	File "..\doc\cmds.txt"
	File "..\doc\SelectionKeys.txt"

	File "..\doc\userdocs\FAQ.html"
	File "..\doc\userdocs\GettingStarted.html"
	File "..\doc\userdocs\Legal.html"
	File "..\doc\userdocs\main.html"
	File "..\doc\userdocs\MoreInfo.html"
!else
	; Documentation
	Delete "$INSTDIR\docs\readme.html"
	Delete "$INSTDIR\docs\LICENSE.html"
	Delete "$INSTDIR\docs\changelog.txt"
	Delete "$INSTDIR\docs\cmds.txt"
	Delete "$INSTDIR\docs\SelectionKeys.txt"
	Delete "$INSTDIR\docs\FAQ.html"
	Delete "$INSTDIR\docs\GettingStarted.html"
	Delete "$INSTDIR\docs\Legal.html"
	Delete "$INSTDIR\docs\main.html"
	Delete "$INSTDIR\docs\MoreInfo.html"
	RMDir  "$INSTDIR\docs"
!endif

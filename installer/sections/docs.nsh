!ifdef INSTALL
!else
	; Documentation
	Delete "$INSTDIR\docs\readme.html"
	Delete "$INSTDIR\docs\LICENSE.html"
	Delete "$INSTDIR\docs\changelog.txt"
	Delete "$INSTDIR\docs\cmds.txt"
	Delete "$INSTDIR\docs\SelectionKeys.txt"
	Delete "$INSTDIR\docs\README.html"
	RMDir  "$INSTDIR\docs"
!endif

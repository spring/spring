!ifdef INSTALL
	SetOutPath "$INSTDIR\docs"
	File "..\LICENSE.html"
	File "..\doc\changelog.txt"
	File "..\doc\cmds.txt"
	File "..\doc\SelectionKeys.txt"

	${If} ${FileExists} "${BUILD_OR_DIST_DIR}\userdocs\README.html"
		File "${BUILD_OR_DIST_DIR}\userdocs\README.html"
	${Else}
		${!echonow} "Warning: ${BUILD_OR_DIST_DIR}\userdocs\README.htm does not exist"
	${EndIf}

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

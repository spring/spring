
!macro createemptyfile _FILE_NAME
	ClearErrors
	FileOpen $0 ${_FILE_NAME} w
	IfErrors done
		FileWrite $0 ""
		FileClose $0
	done:
!macroend

!ifdef INSTALL

	SetOutPath "$INSTDIR"

	!insertmacro createemptyfile "$INSTDIR\springsettings.cfg"
	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		!insertmacro createemptyfile "$INSTDIR\springlobby.conf"
	${EndIf}

!else

	Delete "$INSTDIR\springsettings.cfg"
	Delete "$INSTDIR\springlobby.conf"

!endif

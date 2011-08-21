!ifdef INSTALL

	SetOutPath "$INSTDIR"
	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		!insertmacro createemptyfile "$INSTDIR\springlobby.conf"
	${EndIf}

!else

	Delete "$INSTDIR\springsettings.cfg"
	Delete "$INSTDIR\springlobby.conf"

!endif

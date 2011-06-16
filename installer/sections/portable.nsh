!ifdef INSTALL

	SetOutPath "$INSTDIR"

	!insertmacro createemptyfile "$INSTDIR\springsettings.cfg"
!ifndef SLIM
	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		!insertmacro createemptyfile "$INSTDIR\springlobby.conf"
	${EndIf}

!endif
!else

	Delete "$INSTDIR\springsettings.cfg"
	Delete "$INSTDIR\springlobby.conf"

!endif

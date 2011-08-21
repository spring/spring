!ifdef INSTALL

	SetOutPath "$INSTDIR"
!ifndef SLIM
	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		!insertmacro createemptyfile "$INSTDIR\springlobby.conf"
	${EndIf}

!endif
!else

	Delete "$INSTDIR\springsettings.cfg"
	Delete "$INSTDIR\springlobby.conf"

!endif

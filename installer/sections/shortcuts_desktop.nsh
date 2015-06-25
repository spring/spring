
!ifdef INSTALL

	SetOutPath "$INSTDIR"

	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		CreateShortCut "$DESKTOP\SpringLobby.lnk" "$INSTDIR\springlobby.exe"
	${EndIf}

!else

	Delete "$DESKTOP\SpringLobby.lnk"
	Delete "$DESKTOP\Spring lobby-client Zero-K.lnk"

!endif


!ifdef INSTALL

	SetOutPath "$INSTDIR"

	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		CreateShortCut "$DESKTOP\SpringLobby.lnk" "$INSTDIR\springlobby.exe"
	${EndIf}
	${If} ${SectionIsSelected} ${SEC_ZERO_K_LOBBY}
		CreateShortCut "$DESKTOP\Spring lobby-client Zero-K.lnk" "$INSTDIR\Zero-K.exe" "" "$INSTDIR\Zero-K.ico"
	${EndIf}

!else

	Delete "$DESKTOP\SpringLobby.lnk"
	Delete "$DESKTOP\Zero-K Lobby.lnk"

!endif

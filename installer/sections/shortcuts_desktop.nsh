
!ifdef INSTALL

	SetOutPath "$INSTDIR"

	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		CreateShortCut "$DESKTOP\SpringLobby.lnk" "$INSTDIR\springlobby.exe"
	${EndIf}
	${If} ${SectionIsSelected} ${SEC_ZERO_K_LOBBY}
		CreateShortCut "$DESKTOP\Zero-K Lobby.lnk" "$INSTDIR\Zero-K.exe"
	${EndIf}

!else

	Delete "$DESKTOP\SpringLobby.lnk"
	Delete "$DESKTOP\Zero-K Lobby.lnk"

!endif

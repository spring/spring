
!ifdef INSTALL

	SetOutPath "$INSTDIR"

	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		CreateShortCut "$DESKTOP\SpringLobby.lnk" "$INSTDIR\springlobby.exe"
	${EndIf}
	${If} ${SectionIsSelected} ${SEC_SPRINGDOWNLOADER}
		CreateShortCut "$DESKTOP\SpringDownloader.lnk" "$INSTDIR\SpringDownloader.exe"
	${EndIf}

!else

	Delete "$DESKTOP\SpringLobby.lnk"
	Delete "$DESKTOP\SpringDownloader.lnk"

!endif

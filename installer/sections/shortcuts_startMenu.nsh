
!ifdef INSTALL

	SetOutPath "$INSTDIR"
	; Main shortcuts
	CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
	SetOutPath "$INSTDIR"

	${If} ${SectionIsSelected} ${SEC_SPRINGLOBBY}
		CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\SpringLobby.lnk" "$INSTDIR\springlobby.exe"
	${EndIf}
	${If} ${SectionIsSelected} ${SEC_ZERO_K_LOBBY}
		CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Zero-K Lobby.lnk" "$INSTDIR\Zero-K.exe"
	${EndIf}
	${If} ${SectionIsSelected} ${SEC_RAPID}
		CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\rapid-GUI.lnk" "$INSTDIR\rapid\rapid-gui.exe"
	${EndIf}
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk" "$INSTDIR\springsettings.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk" "$INSTDIR\spring.exe"
	!ifdef SEC_GML
		${If} ${SectionIsSelected} ${SEC_GML}
			CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT.lnk" "$INSTDIR\spring-multithreaded.exe"
		${EndIf}
	!endif

	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Readme.lnk" "$INSTDIR\docs\README.html"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk" "$INSTDIR\uninst.exe"

!else

	; Shortcuts
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\SpringLobby.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Zero-K Lobby.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Download Content.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\rapid-GUI.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Readme.lnk"

	; deprecated files
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring test.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring multiplayer battleroom.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Update CA.lnk"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\SpringDownloader.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring Website.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Download Content.lnk"

	Delete "$INSTDIR\Spring.url"
	Delete "$INSTDIR\springfiles.url"

	; delete the folders
	RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

!endif

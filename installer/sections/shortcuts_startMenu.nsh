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
!ifdef RAPID_ARCHIVE
	${If} ${SectionIsSelected} ${SEC_RAPID}
		CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\rapid-GUI.lnk" "$INSTDIR\rapid\rapid-gui.exe"
	${EndIf}
!endif
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk" "$INSTDIR\springsettings.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk" "$INSTDIR\spring.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring (safemode).lnk" "$INSTDIR\spring.exe" "--safemode"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT.lnk" "$INSTDIR\spring-multithreaded.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT (safemode).lnk" "$INSTDIR\spring-multithreaded.exe" "--safemode"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk" "$INSTDIR\uninst.exe"

	WriteINIStr "$SMPROGRAMS\${PRODUCT_NAME}\Read Me First.URL" "InternetShortcut" "URL" "http://springrts.com/wiki/Read_Me_First"

!else

	; Shortcuts
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\SpringLobby.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Zero-K Lobby.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\rapid-GUI.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring (safemode).lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT (safemode).lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Read Me First.URL"

	; delete the folders
	RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

!endif

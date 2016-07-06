!ifdef INSTALL

	SetOutPath "$INSTDIR"
	; Main shortcuts
	CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
	SetOutPath "$INSTDIR"

	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk" "$INSTDIR\spring.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring (safemode).lnk" "$INSTDIR\spring.exe" "--safemode"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk" "$INSTDIR\uninst.exe"

	WriteINIStr "$SMPROGRAMS\${PRODUCT_NAME}\Read Me First.URL" "InternetShortcut" "URL" "https://springrts.com/wiki/Read_Me_First"

!else

	; Shortcuts
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring (safemode).lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall Spring.lnk"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Read Me First.URL"

	; delete the folders
	RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

!endif

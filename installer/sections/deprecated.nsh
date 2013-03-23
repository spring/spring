!ifdef INSTALL
	; Old DLLs, not needed anymore
	; (python upgraded to 25)
	Delete "$INSTDIR\python24.dll"

	Delete "$INSTDIR\settingstemplate.xml"

	; Remove Luxi.ttf, it has been replaced by FreeSansBold
	Delete "$INSTDIR\Luxi.ttf"
	Delete "$INSTDIR\fonts\Luxi.ttf"

	; Remove SelectionEditor, it has been integrated into SpringLobby and SpringSettings
	Delete "$INSTDIR\SelectionEditor.exe"
	Delete "$INSTDIR\MSVCP71.dll"
	Delete "$INSTDIR\zlibwapi.dll"

	; Purge old file from 0.75 install.
	Delete "$INSTDIR\LuaUI\unitdefs.lua"

	; next one is deprecated since mingwlibs 20.1 (around spring v0.81.2.1)
	Delete "$INSTDIR\wrap_oal.dll"
	Delete "$INSTDIR\cache\ArchiveCacheV9.lua"

	Delete "$INSTDIR\SpringDownloader.exe"
	Delete "$INSTDIR\springfiles.url"
	Delete "$INSTDIR\ArchiveCacheV7.lua"

	RmDir "$INSTDIR\mods"

	; deprecated Shortcuts
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring test.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring multiplayer battleroom.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Selectionkeys editor.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Update CA.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\rapid-GUI.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Settings.lnk"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\SpringDownloader.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Spring Website.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Download Content.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Readme.lnk"

	Delete "$INSTDIR\Spring.url"
	Delete "$INSTDIR\springfiles.url"

	Delete "$DESKTOP\Zero-K Lobby.lnk"

!endif

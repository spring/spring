!ifdef INSTALL
	; 99.0
	Delete "$INSTDIR\Zero-K.exe"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Zero-K Lobby.lnk"
	Delete "$DESKTOP\Spring lobby-client Zero-K.lnk"

	; 95.0
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Test Spring MT (safemode).lnk"
	Delete "$INSTDIR\spring-multithreaded.exe"

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

	; zero-k lobby icon
	Delete "$INSTDIR\Zero-K.ico"

	RmDir "$INSTDIR\mods"

        ; Demofile file association (with multiengine support this doesn't work as current spring can't run old demos)
        !insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"

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

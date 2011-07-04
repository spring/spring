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
!else
	; next one is deprecated since mingwlibs 20.1 (around spring v0.81.2.1)
	Delete "$INSTDIR\wrap_oal.dll"
	Delete "$INSTDIR\vorbisfile.dll"
	Delete "$INSTDIR\vorbis.dll"
	Delete "$INSTDIR\ogg.dll"
	Delete "$INSTDIR\cache\ArchiveCacheV9.lua"
	Delete "$INSTDIR\ArchiveCacheV7.lua"
	; deprecated files
	Delete "$INSTDIR\SpringDownloader.exe"
	Delete "$INSTDIR\springfiles.url"
	Delete "$INSTDIR\springfiles.url"
	Delete "$INSTDIR\ArchiveCacheV7.lua"
	; Old AI stuff
	RmDir /r "$INSTDIR\AI\Global"
	RmDir /r "$INSTDIR\AI\Skirmish"
	RmDir /r "$INSTDIR\AI\Helper-libs"

	RmDir "$INSTDIR\mods"

	; Old Springlobby stuff
        RmDir /R "$INSTDIR\SpringLobbyDocs"
!endif

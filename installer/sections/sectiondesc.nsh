; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_PORTABLE} "This will keep all the configuration local to the install dir. Use when installing on a pen-drive, for example."
!ifndef SLIM
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_SPRINGLOBBY} "The default, cross-platform battleroom used to set up single- and multiplayer games and finding opponents.$\n$\nNote: You have to install at least one Lobby to play."

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_ARCHIVEMOVER} "The tool and the necessary file associations (for sd7 and sdz files) to ease installation of new content."
!ifdef RAPID_ARCHIVE
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_RAPID} "Simple spring-rapid downloader to get latest game versions easily, with console- and GUI-frontend."
!endif
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_ZERO_K_LOBBY} "Fast, easy lobby with automated downloads of maps, games, missions and GUI widgets."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_TASSERVER} "Default spring lobby server, required for LAN play with friends."

!endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END

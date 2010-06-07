; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "The core components required to run Spring. This includes the configuration utilities.$\n$\nNote: This section is required and cannot be deselected."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_GML} "The GML/Multi-Threaded version of the engine."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_SPRINGLOBBY} "The default, cross-platform battleroom used to set up single- and multiplayer games and finding opponents.$\n$\nNote: You must install either TASClient or SpringLobby to play."

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_ARCHIVEMOVER}     "The tool and the necessary file associations (for sd7 and sdz files) to ease installation of new content."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_SPRINGDOWNLOADER} "Helps with auto and manually downloading content for Spring, including Maps, Mods and Lua Widgets; requires .NET or Mono installed."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_TASSERVER} "Default spring lobby server, required for LAN play with friends."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_SELECTIONEDITOR}  "GUI for editing custom selection shortcuts."

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_AAI}    "Supports BA, some *A and other Games; Note: requires manual config file management (author: submarine)"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_KAIK}   "Supports BA and some other *A Games (authors: (current:) Kloot, (former:) Krogothe & Tournesol)"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_RAI}    "Supports BA, some *A and other Games (author: Reth)"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_E323AI} "Supports BA and XTA, uses guerilla tactics (author: Error323)"

	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_START} "This creates shortcuts on the start menu to all the applications provided."
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "This creates a shortcut on the desktop to the multiplayer battleroom for quick access to multiplayer games."

!insertmacro MUI_FUNCTION_DESCRIPTION_END

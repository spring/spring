!ifdef INSTALL

	SetOutPath "$INSTDIR"

	File /r "..\installer\Springlobby\SLArchive\*.*"

!else

	Delete "$INSTDIR\springlobby.exe"
	Delete "$INSTDIR\SDL_mixer.dll"
	Delete "$INSTDIR\wxbase28u_xml_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_aui_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_gl_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_html_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_qa_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_richtext_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_xrc_gcc_custom.dll"
	Delete "$INSTDIR\boost_system-gcc44-mt-1_41.dll"
	Delete "$INSTDIR\boost_thread-gcc44-mt-1_41.dll"
	Delete "$INSTDIR\boost_filesystem-gcc44-mt-1_41.dll"
	Delete "$INSTDIR\springlobby_updater.exe"
	Delete "$INSTDIR\AUTHORS"
	Delete "$INSTDIR\COPYING"
	Delete "$INSTDIR\README"
	Delete "$INSTDIR\NEWS"
	Delete "$INSTDIR\THANKS"
	RmDir /R "$INSTDIR\locale"
	RmDir /R "$INSTDIR\SpringLobbyDocs"

!endif

!ifdef INSTALL
  SetOutPath "$INSTDIR"
  !ifdef TEST_BUILD
   inetc::get \
               "http://www.springlobby.info/installer/svn/springlobby.exe" "$INSTDIR\springlobby.exe"
  !else
   inetc::get \
               "http://www.springlobby.info/installer/springlobby.exe" "$INSTDIR\springlobby.exe"
  !endif
  File "..\external\SDL_mixer.dll"
  File "..\external\wxbase28u_xml_gcc_custom.dll"
  File "..\external\wxmsw28u_aui_gcc_custom.dll"
  File "..\external\wxmsw28u_html_gcc_custom.dll"
  File "..\external\wxmsw28u_qa_gcc_custom.dll"
  File "..\external\wxmsw28u_richtext_gcc_custom.dll"
  SetOutPath "$INSTDIR\SpringLobbyDocs"
  File "..\external\SpringLobbyDocs\AUTHORS"
  File "..\external\SpringLobbyDocs\ChangeLog"
  File "..\external\SpringLobbyDocs\COPYING"
  File "..\external\SpringLobbyDocs\INSTALL"
  File "..\external\SpringLobbyDocs\NEWS"
  File "..\external\SpringLobbyDocs\README"
  File "..\external\SpringLobbyDocs\THANKS"
!else

  Delete "$INSTDIR\springlobby.exe"
  Delete "$INSTDIR\SDL_mixer.dll"
  Delete "$INSTDIR\wxbase28u_xml_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_aui_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_html_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_qa_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_richtext_gcc_custom.dll"
  Delete "$INSTDIR\SpringLobbyDocs\AUTHORS"
  Delete "$INSTDIR\SpringLobbyDocs\ChangeLog"
  Delete "$INSTDIR\SpringLobbyDocs\COPYING"
  Delete "$INSTDIR\SpringLobbyDocs\INSTALL"
  Delete "$INSTDIR\SpringLobbyDocs\NEWS"
  Delete "$INSTDIR\SpringLobbyDocs\README"
  Delete "$INSTDIR\SpringLobbyDocs\THANKS"
  RmDir "$INSTDIR\SpringLobbyDocs"
!endif

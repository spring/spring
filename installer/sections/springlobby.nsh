!ifdef INSTALL
  SetOutPath "$INSTDIR"
  inetc::get \
              "http://www.springlobby.info/installer/springlobby.exe" "$INSTDIR\springlobby.exe"

  File /r "..\installer\Springlobby\SLArchive\*.*"
!else

  Delete "$INSTDIR\springlobby.exe"
  Delete "$INSTDIR\SDL_mixer.dll"
  Delete "$INSTDIR\wxbase28u_xml_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_aui_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_html_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_qa_gcc_custom.dll"
  Delete "$INSTDIR\wxmsw28u_richtext_gcc_custom.dll"
  RmDir /R "$INSTDIR\SpringLobbyDocs"
!endif

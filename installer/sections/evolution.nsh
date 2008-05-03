!ifdef INSTALL
  StrCpy $EVO "true"
  SetOutPath "$INSTDIR\mods"
  ;inetc::get \
  ;           "http://installer.clan-sy.com/mods/EvolutionRTS_Installer_Version.sd7" "$INSTDIR\mods\EvolutionRTS_Installer_Version.sd7" 
  inetc::get \
             "http://installer.clan-sy.com/mods/Evolution_Updater.bat" "$INSTDIR\mods\Evolution_Updater.bat" \
	     "http://installer.clan-sy.com/mods/wget.exe" "$INSTDIR\mods\wget.exe" \
	     "http://installer.clan-sy.com/mods/xdelta3.exe" "$INSTDIR\mods\xdelta3.exe" 
!else

  ;Delete "$INSTDIR\mods\EvolutionRTS_Installer_Version.sd7"
  ExecWait  '"$INSTDIR\mods\Evolution_Updater.bat" uninstall'
  Delete "$INSTDIR\mods\Evolution_Updater.bat"
  Delete "$INSTDIR\mods\wget.exe"
  Delete "$INSTDIR\mods\xdelta3.exe"
  RmDir "$INSTDIR\mods"

!endif

!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/EvolutionRTS_Installer_Version.sd7" "$INSTDIR\mods\EvolutionRTS_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\EvolutionRTS_Installer_Version.sd7"
  RmDir "$INSTDIR\mods"

!endif

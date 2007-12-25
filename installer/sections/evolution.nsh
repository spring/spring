!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://buildbot.no-ip.org/~lordmatt/mods/EvolutionRTS-Chapter1_Installer_Version.sd7" "$INSTDIR\mods\EvolutionRTS-Chapter1_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\EvolutionRTS-Chapter1_Installer_Version.sd7"
  RmDir "$INSTDIR\mods"

!endif

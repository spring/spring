!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://buildbot.no-ip.org/~lordmatt/mods/S44_Installer_Version.sd7" "$INSTDIR\mods\S44_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\S44_Installer_Version.sd7"
  RmDir "$INSTDIR\mods"

!endif

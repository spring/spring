!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://buildbot.no-ip.org/~lordmatt/mods/SimBase_Installer_Version.sd7" "$INSTDIR\mods\SimBase_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\SimBase_Installer_Version.sd7"
  RmDir "$INSTDIR\mods"

!endif

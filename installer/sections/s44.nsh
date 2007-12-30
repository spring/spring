!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/S44_Installer_Version.sdz" "$INSTDIR\mods\S44_Installer_Version.sdz" 

!else

  Delete "$INSTDIR\mods\S44_Installer_Version.sdz"
  RmDir "$INSTDIR\mods"

!endif

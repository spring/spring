!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/SimBase_Installer_Version.sd7" "$INSTDIR\mods\SimBase_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\SimBase_Installer_Version.sd7"
  RmDir "$INSTDIR\mods"

!endif

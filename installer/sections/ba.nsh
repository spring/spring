!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/BA_Installer_Version.sd7" "$INSTDIR\mods\BA_Installer_Version.sd7" 

!else

  Delete "$INSTDIR\mods\BA_Installer_Version.sd7"
  Delete "$INSTDIR\base\tatextures_v062.sdz"
  Delete "$INSTDIR\base\tacontent_v2.sdz"
  Delete "$INSTDIR\base\otacontent.sdz"
  RmDir "$INSTDIR\base"
  RmDir "$INSTDIR\mods"

!endif


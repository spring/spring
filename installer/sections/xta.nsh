!ifdef INSTALL

  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/XTA_Installer_Version.sdz" "$INSTDIR\mods\XTA_Installer_Version.sdz" 

!else

  Delete "$INSTDIR\mods\XTA_Installer_Version.sdz"
  Delete "$INSTDIR\base\tatextures_v062.sdz"
  Delete "$INSTDIR\base\tacontent_v2.sdz"
  Delete "$INSTDIR\base\otacontent.sdz"
  RmDir "$INSTDIR\base"
  RmDir "$INSTDIR\mods"

!endif


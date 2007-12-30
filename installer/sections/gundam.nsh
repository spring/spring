!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/GUNDAM_Installer_Version.sd7" "$INSTDIR\mods\GUNDAM_Installer_Version.sd7" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/gundam/EERiverGladev02.sd7" "$INSTDIR\maps\EERiverGladev02.sd7"

!else

  Delete "$INSTDIR\mods\GUNDAM_Installer_Version.sd7"
  Delete "$INSTDIR\maps\EERiverGladev02.sd7"
  Delete "$INSTDIR\maps\paths\EERiverGladev02.*"
  RmDir "$INSTDIR\mods"
  RmDir "$INSTDIR\maps"

!endif

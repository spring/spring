!ifdef INSTALL
  SetOutPath "$INSTDIR\mods"
  inetc::get \
             "http://installer.clan-sy.com/mods/Kernel_Panic.sd7" "$INSTDIR\mods\Kernel_Panic_Installer_Version.sd7" \
	     "http://installer.clan-sy.com/Kernel_Panic_Launcher.exe" "$INSTDIR\Kernel_Panic_Launcher.exe" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Direct_Memory_Access_0.5c_beta.sd7" "$INSTDIR\maps\Direct_Memory_Access_0.5c_beta.sd7" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Direct_Memory_Access_0.5e_beta.sd7" "$INSTDIR\maps\Direct_Memory_Access_0.5e_beta.sd7" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Major_Madness3.0.sd7" "$INSTDIR\maps\Major_Madness3.0.sd7" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Marble_Madness_Map.sd7" "$INSTDIR\maps\Marble_Madness_Map.sd7" \
	     "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Speed_Balls_16_Way.sdz" "$INSTDIR\maps\Speed_Balls_16_Way.sdz" \
             "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/Spooler_Buffer_0.5_beta.sd7" "$INSTDIR\maps\Spooler_Buffer_0.5_beta.sd7" \
             "http://installer.clan-sy.com/maps/modspecificmaps/kernel_panic/DigitalDivide_PT2.sd7" "$INSTDIR\maps\DigitalDivide_PT2.sd7"              

  SetOutPath "$INSTDIR\AI\NTai"
  inetc::get \
	     "http://installer.clan-sy.com/AI/NTai/Kernel_Panic.tdf" "$INSTDIR\AI\NTai\Kernel_Panic.tdf" 

  SetOutPath "$INSTDIR\AI\NTai\configs"
  inetc::get \
	     "http://installer.clan-sy.com/AI/NTai/configs/Kernel_Panic.tdf" "$INSTDIR\AI\NTai\configs\Kernel_Panic.tdf" 
!else

  Delete "$INSTDIR\mods\Kernel_Panic.sd7"
  Delete "$INSTDIR\mods\Kernel_Panic_Evilless.sd7"
  Delete "$INSTDIR\AI\NTai\configs\Kernel_Panic.tdf"
  Delete "$INSTDIR\AI\NTai\Kernel_Panic.tdf"
  Delete "$INSTDIR\maps\Direct_Memory_Access_0.5c_beta.sd7"
  Delete "$INSTDIR\maps\paths\Direct_Memory_Access_0.5c_beta.*"
  Delete "$INSTDIR\maps\Direct_Memory_Access_0.5e_beta.sd7"
  Delete "$INSTDIR\maps\paths\Direct_Memory_Access_0.5e_beta.*"
  Delete "$INSTDIR\maps\Major_Madness3.0.sd7"
  Delete "$INSTDIR\maps\paths\Major_Madness3.0.*"
  Delete "$INSTDIR\maps\Marble_Madness_Map.sd7"
  Delete "$INSTDIR\maps\paths\Marble_Madness_Map.*"
  Delete "$INSTDIR\maps\Speed_Balls_16_Way.sdz"
  Delete "$INSTDIR\maps\paths\Speed_Balls_16_Way.*"
  RmDir "$INSTDIR\mods"
  RmDir "$INSTDIR\maps"
  RmDir "$INSTDIR\AI\NTai\configs"
  RmDir "$INSTDIR\AI\NTai"
  RmDir "$INSTDIR\AI"
!endif

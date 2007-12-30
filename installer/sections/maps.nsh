!ifdef INSTALL
  SetOutPath "$INSTDIR\maps"  
  inetc::get \
             "http://installer.clan-sy.com/maps/SmallDivide.sd7" "$INSTDIR\maps\SmallDivide.sd7" \
	     "http://installer.clan-sy.com/maps/Comet_Catcher_Redux.sd7" "$INSTDIR\maps\Comet_Catcher_Redux.sd7" \
	     "http://installer.clan-sy.com/maps/Sands_of_War_v2.sd7" "$INSTDIR\maps\Sands_of_War_v2.sd7" 

!else
  ; Maps
  Delete "$INSTDIR\maps\SmallDivide.sd7"
  Delete "$INSTDIR\maps\paths\SmallDivide.*"
  Delete "$INSTDIR\maps\Comet_Catcher_Redux.sd7"
  Delete "$INSTDIR\maps\paths\Comet_Catcher_Redux.*"
  Delete "$INSTDIR\maps\Sands_of_War_v2.sd7"
  Delete "$INSTDIR\maps\paths\Sands_of_War_v2.*"
  RmDir "$INSTDIR\maps\paths"
  RmDir "$INSTDIR\maps"
!endif

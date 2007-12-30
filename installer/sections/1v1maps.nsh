!ifdef INSTALL
  SetOutPath "$INSTDIR\maps"  
  inetc::get \
             "http://installer.clan-sy.com/maps/1v1maps/4886_Lowland_Crossing_TNM01-V4.sd7" "$INSTDIR\maps\4886_Lowland_Crossing_TNM01-V4.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/8862_Cooper_Hill_TNM02-V1.sd7" "$INSTDIR\maps\8862_Cooper_Hill_TNM02-V1.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Altair_Crossing.sd7" "$INSTDIR\maps\Altair_Crossing.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Aquatic_Divide_TNM05-V2.sd7" "$INSTDIR\maps\Aquatic_Divide_TNM05-V2.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/ArcticPlainsV2.sd7" "$INSTDIR\maps\ArcticPlainsV2.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Battle for PlanetXVII-v01.sd7" "$INSTDIR\maps\Battle for PlanetXVII-v01.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Barren.sd7" "$INSTDIR\maps\Barren.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/BlueBend-v01.sd7" "$INSTDIR\maps\BlueBend-v01.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Brazillian_BattlefieldV2.sd7" "$INSTDIR\maps\Brazillian_BattlefieldV2.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/CoastToCoastRemakeV2.sd7" "$INSTDIR\maps\CoastToCoastRemakeV2.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/DarkSide_Remake.sd7" "$INSTDIR\maps\DarkSide_Remake.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Geyser_Plains_TNM04-V3.sd7" "$INSTDIR\maps\Geyser_Plains_TNM04-V3.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/HundredsIsles.sd7" "$INSTDIR\maps\HundredsIsles.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/Mars.sd7" "$INSTDIR\maps\Mars.sd7" \
	     "http://installer.clan-sy.com/maps/1v1maps/MoonQ10x.sd7" "$INSTDIR\maps\MoonQ10x.sd7" 

!else
  ; Maps
  Delete "$INSTDIR\maps\paths\4886_Lowland_Crossing_TNM01-V4.*"
  Delete "$INSTDIR\maps\4886_Lowland_Crossing_TNM01-V4.sd7"
  Delete "$INSTDIR\maps\paths\8862_Cooper_Hill_TNM02-V1.*"
  Delete "$INSTDIR\maps\8862_Cooper_Hill_TNM02-V1.sd7"
  Delete "$INSTDIR\maps\paths\Altair_Crossing.*"
  Delete "$INSTDIR\maps\Altair_Crossing.sd7"
  Delete "$INSTDIR\maps\Aquatic_Divide_TNM05-V2.sd7"
  Delete "$INSTDIR\maps\paths\Aquatic_Divide_TNM05-V2.*"
  Delete "$INSTDIR\maps\ArcticPlainsV2.sd7"
  Delete "$INSTDIR\maps\paths\ArcticPlainsV2.*"
  Delete "$INSTDIR\maps\Battle for PlanetXVII-v01.sd7"
  Delete "$INSTDIR\maps\paths\Battle for PlanetXVII-v01.*"
  Delete "$INSTDIR\maps\BlueBend-v01.sd7"
  Delete "$INSTDIR\maps\paths\Barren.*"
  Delete "$INSTDIR\maps\Barren.sd7"
  Delete "$INSTDIR\maps\paths\BlueBend-v01.*"
  Delete "$INSTDIR\maps\Brazillian_BattlefieldV2.sd7"
  Delete "$INSTDIR\maps\paths\Brazillian_BattlefieldV2.*"
  Delete "$INSTDIR\maps\CoastToCoastRemakeV2.sd7"
  Delete "$INSTDIR\maps\paths\CoastToCoastRemakeV2.*"
  Delete "$INSTDIR\maps\DarkSide_Remake.sd7"
  Delete "$INSTDIR\maps\paths\DarkSide_Remake.*"
  Delete "$INSTDIR\maps\Geyser_Plains_TNM04-V3.sd7"
  Delete "$INSTDIR\maps\paths\Geyser_Plains_TNM04-V3.*"
  Delete "$INSTDIR\maps\HundredsIsles.sd7"
  Delete "$INSTDIR\maps\paths\HundredsIsles.*"
  Delete "$INSTDIR\maps\paths\Mars.*"
  Delete "$INSTDIR\maps\Mars.sd7"
  Delete "$INSTDIR\maps\paths\MoonQ10x.*"
  Delete "$INSTDIR\maps\MoonQ10x.sd7"
  RmDir "$INSTDIR\maps\paths"
  RmDir "$INSTDIR\maps"
!endif

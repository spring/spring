!ifdef INSTALL
  SetOutPath "$INSTDIR\maps"  
  inetc::get \
             "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Altored_Divide.sd7" "$INSTDIR\maps\Altored_Divide.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Charlie_in_the_Hills.sd7" "$INSTDIR\maps\Charlie_in_the_Hills.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Crossing_4_final.sd7" "$INSTDIR\maps\Crossing_4_final.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/FrostBiteV2.sd7" "$INSTDIR\maps\FrostBiteV2.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/GreenHaven_Remake.sd7" "$INSTDIR\maps\GreenHaven_Remake.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Islands_In_War_1.0.SDZ" "$INSTDIR\maps\Islands_In_War_1.0.sdz" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/MoonQ20x.sd7" "$INSTDIR\maps\MoonQ20x.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/River_Dale-V01(onlyRiverdale).sd7" "$INSTDIR\maps\River_Dale-V01(onlyRiverdale).sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Small_Supreme_Battlefield_V2.sd7" "$INSTDIR\maps\Small_Supreme_Battlefield_V2.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Tabula-v2.sd7" "$INSTDIR\maps\Tabula-v2.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Victoria_Crater.sd7" "$INSTDIR\maps\Victoria_Crater.sd7" \
	     "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/XantheTerra_v3.sd7" "$INSTDIR\maps\XantheTerra_v3.sd7" \
             "http://buildbot.no-ip.org/~lordmatt/maps/teammaps/Zeus05_A.sd7" "$INSTDIR\maps\Zeus05_A.sd7" 

!else
  ; Maps
  Delete "$INSTDIR\maps\paths\Altored_Divide.*"
  Delete "$INSTDIR\maps\Altored_Divide.sd7"
  Delete "$INSTDIR\maps\paths\Charlie_in_the_Hills.*"
  Delete "$INSTDIR\maps\Charlie_in_the_Hills.sd7"
  Delete "$INSTDIR\maps\paths\Crossing_4_final.*"
  Delete "$INSTDIR\maps\Crossing_4_final.sd7"
  Delete "$INSTDIR\maps\paths\FrostBiteV2.*"
  Delete "$INSTDIR\maps\FrostBiteV2.sd7"
  Delete "$INSTDIR\maps\paths\GreenHaven_Remake.*"
  Delete "$INSTDIR\maps\GreenHaven_Remake.sd7"
  Delete "$INSTDIR\maps\paths\Islands_In_War_1.0.*"
  Delete "$INSTDIR\maps\Islands_In_War_1.0.sdz"
  Delete "$INSTDIR\maps\paths\MoonQ20x.*"
  Delete "$INSTDIR\maps\MoonQ20x.sd7"
  Delete "$INSTDIR\maps\paths\Islands_In_War_1.0.*"
  Delete "$INSTDIR\maps\Islands_In_War_1.0.sdz"
  Delete "$INSTDIR\maps\paths\River_Dale-V01(onlyRiverdale).*"
  Delete "$INSTDIR\maps\River_Dale-V01(onlyRiverdale).sd7"
  Delete "$INSTDIR\maps\paths\Small_Supreme_Battlefield_V2.*"
  Delete "$INSTDIR\maps\Small_Supreme_Battlefield_V2.sd7"
  Delete "$INSTDIR\maps\paths\Tabula-v2.*"
  Delete "$INSTDIR\maps\Tabula-v2.sd7"
  Delete "$INSTDIR\maps\paths\Victoria_Crater.*"
  Delete "$INSTDIR\maps\Victoria_Crater.sd7"
  Delete "$INSTDIR\maps\paths\XantheTerra_v3.*"
  Delete "$INSTDIR\maps\XantheTerra_v3.sd7"
  Delete "$INSTDIR\maps\paths\Zeus05_A.*"
  Delete "$INSTDIR\maps\Zeus05_A.sd7"
  RmDir "$INSTDIR\maps\paths"
  RmDir "$INSTDIR\maps"
!endif

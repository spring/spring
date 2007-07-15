!ifdef INSTALL
  SetOutPath "$INSTDIR\maps"  
  File "..\game\maps\SmallDivide.sd7"
  File "..\game\maps\Mars.sd7"
  File "..\game\maps\Islands_In_War_1.0.sdz"
!else
  ; Maps
  Delete "$INSTDIR\maps\paths\SmallDivide.*"
  Delete "$INSTDIR\maps\paths\Mars.*"
  Delete "$INSTDIR\maps\paths\Islands_In_War_1.0.*"
  Delete "$INSTDIR\maps\SmallDivide.sd7"
  Delete "$INSTDIR\maps\Mars.sd7"
  Delete "$INSTDIR\maps\Islands_In_War_1.0.sdz"
  RmDir "$INSTDIR\maps\paths"
  RmDir "$INSTDIR\maps"
!endif

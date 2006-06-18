!ifdef INSTALL
  SetOutPath "$INSTDIR\maps"  
  File "..\game\maps\SmallDivide.sd7"
  File "..\game\maps\Mars.sd7"
!else
  ; Maps
  Delete "$INSTDIR\maps\paths\SmallDivide.*"
  Delete "$INSTDIR\maps\paths\Mars.*"
  Delete "$INSTDIR\maps\SmallDivide.*"
  Delete "$INSTDIR\maps\FloodedDesert.*"
  Delete "$INSTDIR\maps\Mars.*"
  RmDir "$INSTDIR\maps\paths"
  RMDir "$INSTDIR\maps"
!endif

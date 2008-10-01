!ifdef INSTALL
  SetOutPath "$INSTDIR\AI\Skirmish/impls"
  File "..\game\AI\Skirmish/impls\AAI.dll"

  SetOutPath "$INSTDIR\AI\AAI"
  File /r /x .svn "..\game\AI\AAI\*.*"
  
  ; This should perhaps be removed in the future?
  Delete "$INSTDIR\AI\AAI\log\BuildTable_XTAPE.txt"
  Delete "$INSTDIR\AI\AAI\learn\mod\XTAPE.dat"
  
!else
  Delete "$INSTDIR\AI\Skirmish/impls\AAI.dll"
  RmDir /r "$INSTDIR\AI\AAI"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Skirmish/impls"
  RmDir "$INSTDIR\AI"
!endif

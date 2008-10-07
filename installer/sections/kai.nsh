!ifdef INSTALL

  SetOutPath "$INSTDIR\AI\Skirmish\impls"
  File "..\game\AI\Skirmish\impls\KAIK-0.13.dll"

  ; Delete the old KAI.
  Delete "$INSTDIR\AI\Skirmish\impls\KAI-0.12.dll"

!else

  Delete "$INSTDIR\AI\Skirmish\impls\KAIK-0.13.dll"
  RmDir /r "$INSTDIR\AI\KAIK013"
  ; we run after main.nsh so need to remove dirs here..
  RmDir "$INSTDIR\AI\Skirmish\impls"
  RmDir "$INSTDIR\AI"

!endif

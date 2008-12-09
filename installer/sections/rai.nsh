!ifdef INSTALL
	; Installing:
;	; Delete the old RAI.
;	RmDir /r "$INSTDIR\AI\Skirmish\RAI"

	SetOutPath "$INSTDIR\AI\Skirmish"
	File /r /x *.a /x *.def "..\game\AI\Skirmish\RAI"
!else
	; Uninstalling:
	RmDir /r "$INSTDIR\AI\Skirmish\RAI\0.553"
!endif

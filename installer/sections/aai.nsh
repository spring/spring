!ifdef INSTALL
	; Installing:
;	; Delete the old AAI.
;	RmDir /r "$INSTDIR\AI\Skirmish\AAI"
	RmDir /r "$INSTDIR\AI\AAI"

	SetOutPath "$INSTDIR\AI\Skirmish"
	File /r /x *.a /x *.def "..\game\AI\Skirmish\AAI"
!else
	; Uninstalling:
	RmDir /r "$INSTDIR\AI\Skirmish\AAI\0.777"
	RmDir /r "$INSTDIR\AI\AAI"
!endif

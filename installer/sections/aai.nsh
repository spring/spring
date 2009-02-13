!ifdef INSTALL
	; Installing:
;	; Delete the old AAI.
;	RmDir /r "$INSTDIR\AI\Skirmish\AAI"
;	RmDir /r "$INSTDIR\AI\AAI"
;	CreateDirectory "$INSTDIR\AI\Skirmish\AAI"
;	CopyFiles "$INSTDIR\AI\AAI\*.*" "$INSTDIR\AI\Skirmish\AAI\*.*"

;	SetOutPath "$INSTDIR\AI\Skirmish"
;	File /r /x *.a /x *.def "..\game\AI\Skirmish\AAI"
	!insertmacro InstallSkirmishAI "AAI"
!else
	; Uninstalling:
;	RmDir /r "$INSTDIR\AI\Skirmish\AAI"
	!insertmacro DeleteSkirmishAI "AAI"
;	RmDir /r "$INSTDIR\AI\AAI"
!endif

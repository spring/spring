!ifdef INSTALL
	; Installing:
	; Delete the old RAI.
;	RmDir /r "$INSTDIR\AI\Skirmish\RAI\0.553"
;	RmDir /r "$INSTDIR\AI\Skirmish\RAI"

	!insertmacro InstallSkirmishAI "RAI"
!else
	; Uninstalling:
	!insertmacro DeleteSkirmishAI "RAI"
;	RmDir /r "$INSTDIR\AI\Skirmish\RAI\0.553"
!endif

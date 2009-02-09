!ifdef INSTALL
	; Installing:
	; Delete the old KAIK.
	RmDir /r "$INSTDIR\AI\KAIK013"
;	RmDir /r "$INSTDIR\AI\Skirmish\KAIK"

	!insertmacro InstallSkirmishAI "KAIK"
!else
	; Uninstalling:
	!insertmacro DeleteSkirmishAI "KAIK"
	RmDir /r "$INSTDIR\AI\KAIK013"
!endif

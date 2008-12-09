!ifdef INSTALL
	; Installing:
	; Delete the old KAIK.
	RmDir /r "$INSTDIR\AI\KAIK013"
	RmDir /r "$INSTDIR\AI\Skirmish\KAIK"

	SetOutPath "$INSTDIR\AI\Skirmish"
	File /r /x *.a /x *.def "..\game\AI\Skirmish\KAIK"
!else
	; Uninstalling:
	RmDir /r "$INSTDIR\AI\KAIK013"
	RmDir /r "$INSTDIR\AI\Skirmish\KAIK\0.12"
!endif

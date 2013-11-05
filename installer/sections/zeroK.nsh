!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File "..\installer\downloads\Zero-K.exe"

	; This is not required anymore.
	; The installed Zero-K.exe (setup) will check
	; for the required .Net version its self.
	;Call EnsureZeroKDotNetVersion

!else

	Delete "$INSTDIR\Zero-K.exe"

!endif ; !INSTALL

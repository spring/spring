!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File /r "..\installer\downloads\Zero-K.exe"

	Call EnsureZeroKDotNetVersion

!else

	ExecWait "$INSTDIR\Zero-K.exe -uninstall"
	Delete "$INSTDIR\Zero-K.exe"

!endif ; !INSTALL

!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File "/oname=Zero-K.exe" "..\installer\downloads\setup.exe"

	Call EnsureZeroKDotNetVersion

!else

	ExecWait "$INSTDIR\Zero-K.exe -uninstall"
	Delete "$INSTDIR\Zero-K.exe"

!endif ; !INSTALL

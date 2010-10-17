!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File "/oname=Zero-K.exe" "..\installer\downloads\setup.exe"

	Call EnsureZeroKDotNetVersion

!else

	Delete "$INSTDIR\Zero-K.exe"

!endif ; !INSTALL

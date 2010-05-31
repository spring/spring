!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File /r "..\installer\downloads\SelectionEditor\*.*"

!else

	Delete "$INSTDIR\SelectionEditor.exe"
	Delete "$INSTDIR\MSVCP71.dll"
	Delete "$INSTDIR\MSVCR71.dll"
	Delete "$INSTDIR\zlibwapi.dll"

!endif ; !INSTALL

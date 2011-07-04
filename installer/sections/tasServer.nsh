!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File "downloads\TASServer.jar"

!else

	Delete "$INSTDIR\TASServer.jar"

!endif ; !INSTALL

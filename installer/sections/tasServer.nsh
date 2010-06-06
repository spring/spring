!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File /r "downloads\TASServer.jar"

!else

	Delete "$INSTDIR\TASServer.jar"

!endif ; !INSTALL

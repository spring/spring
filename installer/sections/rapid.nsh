!ifdef INSTALL

	SetOutPath "$INSTDIR"
	File "downloads\rapid-spring-latest-win32.7z"

	SetOutPath "$INSTDIR\rapid\"
	Nsis7z::Extract "$INSTDIR\rapid-spring-latest-win32.7z"
	Delete "$INSTDIR\rapid-spring-latest-win32.7z"

!else

	Delete "$INSTDIR\rapid\*.*"
	Delete "$INSTDIR\rapid"

!endif ; !INSTALL

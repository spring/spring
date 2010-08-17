!ifdef INSTALL
	SetOutPath "$INSTDIR"

	File /r "..\installer\downloads\SpringDownloader.exe"

	Call EnsureSpringDownloaderDotNetVersion

!else

	ExecWait "$INSTDIR\SpringDownloader.exe -uninstall"
	Delete "$INSTDIR\SpringDownloader.exe"

!endif ; !INSTALL

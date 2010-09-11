!include "include\checkDotNetInstalled.nsh"

Function EnsureSpringDownloaderDotNetVersion
	StrCpy $R7 "3.5"

	Push "$R7"
	Call IsMinDotNetVersionInstalled

	Pop $0
	${If} $0 != 0 ; required or newer version is not installed
		Push "SpringDownloader"
		Push "$R7"
		Call InsufficientDotNet
	${EndIf}
FunctionEnd

Function InsufficientDotNet
	Pop $R5 ; required version
	Pop $R6 ; application name

	Call GetInstalledDotNetVersion
	Pop $0

	${If} $0 == "" ; .Net is not installed
		StrCpy $1 "You do not have .NET installed"
	${Else}
		StrCpy $1 "You have only v$0 installed"
	${EndIf}

	MessageBox MB_YESNO \
			"The .NET runtime library v$R5 or newer is required for $R6. $1. Do you wish to download and install it?" \
			IDYES true IDNO false
	true:
		Call DownloadAndInstallLatestDotNet
		Goto next
	false:
	next:
FunctionEnd

Function DownloadAndInstallLatestDotNet
	inetc::get "http://springrts.com/dl/dotnetfx.exe" "$INSTDIR\dotnetfx.exe"
	ExecWait "$INSTDIR\dotnetfx.exe"
	Delete   "$INSTDIR\dotnetfx.exe"
FunctionEnd


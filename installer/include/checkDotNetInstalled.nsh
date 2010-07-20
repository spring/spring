; See for documentation:
; http://www.blog.lcp.com.ar/coding/2007/06/07/how-to-detect-net-framework-with-nsis/

!include "include/Registry.nsh"

; For all these functions:
; pushes 0 for true, -1 for false


!macro IsDotNetVersionInstalled _DOT_NET_VERSION
	!tempfile _TEMPFILE
	!if _DOT_NET_VERSION == "1.0"
		Call IsDotNet10Installed
	!else if _DOT_NET_VERSION == "1.1"
		Call IsDotNet11Installed
	!else if _DOT_NET_VERSION == "2.0"
		Call IsDotNet20Installed
	!else if _DOT_NET_VERSION == "3.0"
		Call IsDotNet30Installed
	!else if _DOT_NET_VERSION == "3.5"
		Call IsDotNet35Installed
	!else
		!error "The specified .Net version is not supported by this script: '${_DOT_NET_VERSION}'"
	!endif
!macroend


Function IsDotNetInstalled
	Call GetInstalledDotNetVersion
	Pop $0

	${If} $0 == "0.0" ; .Net is not installed
		Push -1
	${Else}
		Push 0
	${EndIf}
FunctionEnd

Function IsMinDotNetVersionInstalled
	Pop $R0

	Call GetInstalledDotNetVersion
	Pop $0

	${VersionCompare} $0 $R0 $1

	${If} $1 == 1 ; $0 is newer then $R0 -> too old version is installed
		Push -1
	${Else}
		Push 0
	${EndIf}
FunctionEnd

Function GetInstalledDotNetVersion

	Call IsDotNet10Installed
	Pop $0

	Call IsDotNet11Installed
	Pop $1

	Call IsDotNet20Installed
	Pop $2

	Call IsDotNet30Installed
	Pop $3

	Call IsDotNet35Installed
	Pop $4

	${If} $0 == 0
		Push "1.0"
	${ElseIf} $1 == 0
		Push "1.1"
	${ElseIf} $2 == 0
		Push "2.0"
	${ElseIf} $3 == 0
		Push "3.0"
	${ElseIf} $4 == 0
		Push "3.5"
	${Else}
		Push "0.0"
	${EndIf}
FunctionEnd


!macro checkR1Hyphen
	${If} $R1 == ''
		Push -1
	${Else}
		Push 0
	${EndIf}
!macroend

; Check if .Net 1.0 is installed
Function IsDotNet10Installed
	${registry::Read} "HKEY_LOCAL_MACHINE\Software\Microsoft\.NETFramework\Policy\v1.0\" "3705" $R0 $R1
	!insertmacro checkR1Hyphen
FunctionEnd

; Check if .Net 1.1 is installed
Function IsDotNet11Installed
	${registry::Read} "HKEY_LOCAL_MACHINE\Software\Microsoft\NET Framework Setup\NDP\v1.1.4322\" "Install" $R0 $R1
	!insertmacro checkR1Hyphen
FunctionEnd

; Check if .Net 2.0 is installed
Function IsDotNet20Installed
	${registry::Read} "HKEY_LOCAL_MACHINE\Software\Microsoft\NET Framework Setup\NDP\v2.0.50727" "SP" $R0 $R1
	!insertmacro checkR1Hyphen
FunctionEnd

; Check if .Net 3.0 is installed
Function IsDotNet30Installed
	${registry::Read} "HKEY_LOCAL_MACHINE\Software\Microsoft\NET Framework Setup\NDP\v3.0\Setup\" "InstallSuccess" $R0 $R1
	!insertmacro checkR1Hyphen
FunctionEnd

; Check if .Net 3.5 is installed
Function IsDotNet35Installed
	${registry::Read} "HKEY_LOCAL_MACHINE\Software\Microsoft\NET Framework Setup\NDP\v3.5" "SP" $R0 $R1
	!insertmacro checkR1Hyphen
FunctionEnd


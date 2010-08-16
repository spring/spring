; See for documentation:
; http://www.blog.lcp.com.ar/coding/2007/06/07/how-to-detect-net-framework-with-nsis/

!include "include\Registry.nsh"

; For this functions:
; pushes 0 for true, -1 for false
Function IsMinDotNetVersionInstalled
	Pop $R0

	Call GetInstalledDotNetVersion
	Pop $0

	${VersionCompare} $0 $R0 $1

	${If} $1 < 2 ;Our version is equal or newer
		Push 0
	${Else}
		Push -1
	${EndIf}
FunctionEnd

Function GetInstalledDotNetVersion
	Push $0
	Push $1
	Push $2
	Push $3
	Push $4
	Push $R1 ;Output
	Push $R2 ;Version difference

	StrCpy $R2 0

	ReadRegStr $4 HKEY_LOCAL_MACHINE \
	"Software\Microsoft\.NETFramework" "InstallRoot"
	; remove trailing back slash
	Push $4
	Exch $EXEDIR
	Exch $EXEDIR
	Pop $4
	; if the root directory does not exist .NET is not installed
	IfFileExists $4 0 done

	StrCpy $0 0

	EnumStart:

	EnumRegKey $2 HKEY_LOCAL_MACHINE \
	"Software\Microsoft\.NETFramework\Policy" $0
	IntOp $0 $0 + 1
	StrCmp $2 "" done
	StrCpy $1 0

	EnumPolicy:

	EnumRegValue $3 HKEY_LOCAL_MACHINE \
	"Software\Microsoft\.NETFramework\Policy\$2" $1
	IntOp $1 $1 + 1
	StrCmp $3 "" EnumStart
	IfFileExists "$4\$2.$3" foundDotNET EnumPolicy

	StrCmp $R1 "" onNETNotFound
	Goto onNETFound

	foundDotNET:
	StrCpy $2 $2 "" 1 ;strip 'v'
	
	DetailPrint "Found .NET v$2"
	${VersionCompare} $2 $R1 $R2
	${If} $R2 < 2
		StrCpy $R1 $2
	${EndIf}
	Goto EnumPolicy

	onNETFound:
	DetailPrint "Highest version is .NET v$R1"
	Goto done

	onNETNotFound:
	DetailPrint "Could not find any installed .NET"

	done:
	StrCpy $0 $R1

	Pop $R2
	Pop $R1
	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Exch $0
FunctionEnd



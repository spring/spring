
!macro createemptyfile _FILE_NAME
	ClearErrors
	FileOpen $0 ${_FILE_NAME} w
	IfErrors +3
		FileWrite $0 ""
		FileClose $0
!macroend

; Usage:
;!insertmacro createemptyfile "$INSTDIR\template.txt"

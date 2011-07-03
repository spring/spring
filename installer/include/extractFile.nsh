
; install .7z instdir + extract + delete .7z

!macro extractFile filename tempfile path
	File /oname=${tempfile} "${filename}"
	${If} "${path}" != ""
		SetOutPath "$INSTDIR\${path}"
	${EndIf}
	Nsis7z::Extract "$INSTDIR\${tempfile}"
	${If} "${path}" != ""
		SetOutPath "$INSTDIR"
	${EndIf}
	Delete "$INSTDIR\${tempfile}"
!macroend

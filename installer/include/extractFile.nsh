
; install .7z instdir + extract + delete .7z

!macro extractFile filename tempfile
	File /oname=${tempfile} "${filename}"
	Nsis7z::Extract "$INSTDIR\${tempfile}"
	Delete "$INSTDIR\${tempfile}"
!macroend

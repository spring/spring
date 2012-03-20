!ifdef INSTALL
	File /oname=vcredist_x86.exe ${VCREDIST}
	ExecWait '"$INSTDIR\vcredist_x86.exe" /q'
!else
	Delete "$INSTDIR\vcredist_x86.exe"
!endif

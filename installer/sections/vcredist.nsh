!ifdef INSTALL
	File /oname=vcredist_x86.exe ${VCREDIST}
	${IfNot} ${SectionIsSelected} ${SEC_PORTABLE}
		; install vcredist only on non-portal installs
		ExecWait '"$INSTDIR\vcredist_x86.exe" /q'
	${EndIF}
!else
	Delete "$INSTDIR\vcredist_x86.exe"
!endif

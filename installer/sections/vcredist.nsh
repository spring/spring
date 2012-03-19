!ifdef INSTALL
	File ${VCREDIST}
	ExecWait '"${VCREDIST}" /q:a'
!else
	Delete ${VCREDIST}
!endif

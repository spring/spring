!ifdef INSTALL
	File ${VCREDIST}
	ExecWait '"${VCREDIST}" /q:a /c:"msiexec /i vcredist.msi /qn'
!else
	Delete ${VCREDIST}
!endif

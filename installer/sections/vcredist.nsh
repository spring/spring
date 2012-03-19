!ifdef INSTALL
	File ${VCREDIST}
	ExecWait '"${VCREDIST}" /q:a /C:"VCREDI~3.EXE /q:a /C:""msiexec /i vcredist.msi /qn"""'
!else
	Delete ${VCREDIST}
!endif

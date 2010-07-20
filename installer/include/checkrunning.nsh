
Function CheckExecutableRunningFunc
	Pop $R1 ; executable
	Pop $R2 ; human-name

	begin:
		FindProcDLL::FindProc "$R1"
		; $R0 == 1: process found
		;        0: process not found
		;       >1: some error, assume everything's ok
		IntCmp $R0 1 do_abort proceed proceed
	do_abort:
		MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Please close application '$R2' ('$R1') before installing." IDRETRY begin
		Abort
	proceed:
		Return
FunctionEnd


!macro Call_CheckExecutableRunningFunc _EXECUTABLE _HUMAN_NAME
	Push "${_HUMAN_NAME}"
	Push "${_EXECUTABLE}"
	Call CheckExecutableRunningFunc
!macroend
!define CheckExecutableRunning "!insertmacro Call_CheckExecutableRunningFunc"


; Usage:
;${CheckExecutableRunning} "springsettings.exe" "Spring Settings"


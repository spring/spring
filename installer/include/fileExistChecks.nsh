; See http://nsis.sourceforge.net/Check_if_a_file_exists_at_compile_time for documentation

!macro !defineifexist _VAR_NAME _FILE_NAME
	!tempfile _TEMPFILE
	!ifdef NSIS_WIN32_MAKENSIS
		; Windows - cmd.exe
		!system 'if exist "${_FILE_NAME}" echo !define ${_VAR_NAME} > "${_TEMPFILE}"'
	!else
		; Posix - sh
		!system 'if [ -e "${_FILE_NAME}" ]; then echo "!define ${_VAR_NAME}" > "${_TEMPFILE}"; fi'
	!endif
	!include '${_TEMPFILE}'
	!delfile '${_TEMPFILE}'
	!undef _TEMPFILE
!macroend
!define !defineifexist "!insertmacro !defineifexist"


; Usage:
;${!defineifexist} MY_TEST_VAR  "/file/to/check"
;!ifdef MY_TEST_VAR
;	; do something here
;	!undef MY_TEST_VAR
;!endif

; See http://nsis.sourceforge.net/Check_if_a_file_exists_at_compile_time for documentation

!ifdef OS_WIN
	!ifdef OS_NIX
		!error "Both OS_WIN and OS_NIX are defined!"
	!endif
!endif
!ifndef OS_WIN
	!ifndef OS_NIX
		!error "Neither OS_WIN nor OS_NIX are defined!"
	!endif
!endif

!macro !defineifexist _VAR_NAME _FILE_NAME
	!tempfile _TEMPFILE
	!ifdef OS_WIN
		!system 'if exist "${_FILE_NAME}" echo !define ${_VAR_NAME} > "${_TEMPFILE}"'
	!else
		!system 'if [ -e "${_FILE_NAME}" ]; then echo "!define ${_VAR_NAME}" > "${_TEMPFILE}"; fi'
	!endif
	!include '${_TEMPFILE}'
	!delfile '${_TEMPFILE}'
	!undef _TEMPFILE
!macroend
!define !defineifexist "!insertmacro !defineifexist"

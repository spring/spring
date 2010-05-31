; See http://nsis.sourceforge.net/Check_if_a_file_exists_at_compile_time for documentation

!macro !defineiffileexists _VAR_NAME _FILE_NAME
	!tempfile _TEMPFILE
	!ifdef NSIS_WIN32_MAKENSIS
		; Windows - cmd.exe
		!system 'if exist "${_FILE_NAME}" echo !define ${_VAR_NAME} > "${_TEMPFILE}"'
	!else
		; Posix - sh
		!system 'if [ -e "$(echo "${_FILE_NAME}" | sed -e "s_\\_\/_g")" ]; then echo "!define ${_VAR_NAME}" > "${_TEMPFILE}"; fi'
	!endif
	!include '${_TEMPFILE}'
	!delfile '${_TEMPFILE}'
	!undef _TEMPFILE
!macroend
!define !defineiffileexists "!insertmacro !defineiffileexists"


!macro !defineifdirexists _VAR_NAME _FILE_NAME
	!tempfile _TEMPFILE
	!ifdef NSIS_WIN32_MAKENSIS
		; Windows - cmd.exe
		!system 'if exist "${_FILE_NAME}\*.*" echo !define ${_VAR_NAME} > "${_TEMPFILE}"'
	!else
		; Posix - sh
		!system 'if [ -d "$(echo "${_FILE_NAME}" | sed -e "s_\\_\/_g")" ]; then echo "!define ${_VAR_NAME}" > "${_TEMPFILE}"; fi'
	!endif
	!include '${_TEMPFILE}'
	!delfile '${_TEMPFILE}'
	!undef _TEMPFILE
!macroend
!define !defineifdirexists "!insertmacro !defineifdirexists"


; Usage:
;${!defineiffileexists} FILE_EXISTS ".\to\check\file.txt"
;!ifdef FILE_EXISTS
;	!system 'echo "The file exists!"'
;	!undef FILE_EXISTS
;!endif
;
;${!defineifdirexists} DIR_EXISTS ".\to\check\dir"
;!ifdef DIR_EXISTS
;	!system 'echo "The dir exists!"'
;	!undef DIR_EXISTS
;!endif

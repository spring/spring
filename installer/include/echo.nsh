; A cross-platform compile-time echo command.
; This should do what `!echo` is meant to do, but this works under linux.

!macro !compile_time_echo _MSG
	!ifdef NSIS_WIN32_MAKENSIS
		; Windows - cmd.exe
		!system 'echo ${_MSG}'
	!else
		; Posix - sh
		!system 'echo "${_MSG}"'
	!endif
!macroend
!define !echonow "!insertmacro !compile_time_echo"


; Usage:
;${!echonow} "Packaging the installer now ..."


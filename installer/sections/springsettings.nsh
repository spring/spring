!ifndef SLIM
!ifdef INSTALL
	; New Settings Program
	${!echonow} "Processing: main: springsettings"
	File "..\installer\Springlobby\SettingsDlls\springsettings.exe"
	File /r "..\installer\Springlobby\SettingsDlls\*.dll"
!else

	; New Settings Program
	Delete "$INSTDIR\springsettings.exe"
	Delete "$INSTDIR\mingwm10.dll"
	Delete "$INSTDIR\wxbase28u_gcc_custom.dll"
	Delete "$INSTDIR\wxbase28u_net_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_adv_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_core_gcc_custom.dll"
	Delete "$INSTDIR\springsettings.conf"

!endif
!endif


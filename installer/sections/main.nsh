
!ifdef INSTALL

	SetOutPath "$INSTDIR"
	SetOverWrite on

	; Main stuff
	${!echonow} "Processing: main: spring.exe & unitsync.dll"
	File "${BUILD_OR_DIST_DIR}\spring.exe"
	File "${BUILD_OR_DIST_DIR}\unitsync.dll"
	CreateDirectory "$INSTDIR\maps"
	CreateDirectory "$INSTDIR\mods"
	SetOutPath "$INSTDIR"

	${!echonow} "Processing: main: DLLs (mingwlibs)"
	; DLLs (updated in mingwlibs-v20.1)
	File "${MINGWLIBS_DIR}\dll\glew32.dll"
	File "${MINGWLIBS_DIR}\dll\python25.dll"
	File "${MINGWLIBS_DIR}\dll\zlib1.dll"
	File "${MINGWLIBS_DIR}\dll\OpenAL32.dll"
	File "${MINGWLIBS_DIR}\dll\vorbisfile.dll"
	File "${MINGWLIBS_DIR}\dll\vorbis.dll"
	File "${MINGWLIBS_DIR}\dll\ogg.dll"
	File "${MINGWLIBS_DIR}\dll\libgcc_s_dw2-1.dll"

	; http://msdn.microsoft.com/en-us/library/abx4dbyh(VS.71).aspx
	; on that page, see:
	; "What is the difference between msvcrt.dll and msvcr71.dll?"
	File "${MINGWLIBS_DIR}\dll\MSVCR71.dll"

	; Use SDL 1.2.10 because SDL 1.2.{9,11,12} break keyboard layout.
	File "${MINGWLIBS_DIR}\dll\SDL.dll"

	; Old DLLs, not needed anymore
	; (python upgraded to 25)
	Delete "$INSTDIR\python24.dll"

	Delete "$INSTDIR\settingstemplate.xml"

	; New Settings Program
	${!echonow} "Processing: main: springsettings"
	File "..\installer\Springlobby\SettingsDlls\springsettings.exe"
	File /r "..\installer\Springlobby\SettingsDlls\*.dll"

	${!echonow} "Processing: main: DLLs 2 (mingwlibs)"
	; DLLs
	File "${MINGWLIBS_DIR}\dll\DevIL.dll"
	File "${MINGWLIBS_DIR}\dll\freetype6.dll"
	File "${MINGWLIBS_DIR}\dll\ILU.dll"


	${!echonow} "Processing: main: content"
	File "${CONTENT_DIR}\PALETTE.PAL"

	${IfNot} ${FileExists} "$INSTDIR\selectkeys.txt"
		File "${CONTENT_DIR}\selectkeys.txt"
	${EndIf}

	${IfNot} ${FileExists} "$INSTDIR\uikeys.txt"
		File "${CONTENT_DIR}\uikeys.txt"
	${EndIf}

	${IfNot} ${FileExists} "$INSTDIR\cmdcolors.txt"
		File "${CONTENT_DIR}\cmdcolors.txt"
	${EndIf}

	${IfNot} ${FileExists} "$INSTDIR\ctrlpanel.txt"
		File "${CONTENT_DIR}\ctrlpanel.txt"
	${EndIf}

	${IfNot} ${FileExists} "$INSTDIR\teamcolors.lua"
		File "${CONTENT_DIR}\teamcolors.lua"
	${EndIf}

	SetOutPath "$INSTDIR\fonts"
	File "${CONTENT_DIR}\fonts\FreeSansBold.otf"

	; Remove Luxi.ttf, it has been replaced by FreeSansBold
	Delete "$INSTDIR\Luxi.ttf"
	Delete "$INSTDIR\fonts\Luxi.ttf"

	; Remove SelectionEditor, it has been integrated into SpringLobby and SpringSettings
	Delete "$INSTDIR\SelectionEditor.exe"
	Delete "$INSTDIR\MSVCP71.dll"
	Delete "$INSTDIR\zlibwapi.dll"

  ; AI Interfaces
!macro InstallAIInterface aiIntName
!ifdef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
  SetOutPath "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
  !ifdef USE_BUILD_DIR
	File /r /x *.a /x *.def /x *.7z /x *.dbg /x CMakeFiles /x Makefile /x cmake_install.cmake "${BUILD_DIR}\AI\Interfaces\${aiIntName}\*.*"
	File /r "..\AI\Interfaces\${aiIntName}\data\*.*"
  !else
	File /r /x *.a /x *.def /x *.7z /x *.dbg "${DIST_DIR}\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
  !endif
  ;buildbot creates 7z, and those get included in installer, fix here until buildserv got fixed
  !undef AI_INT_VERS
!endif
!macroend
	${!echonow} "Processing: main: AI Interfaces"
	!insertmacro InstallAIInterface "C"
	!insertmacro InstallAIInterface "Java"
	!insertmacro InstallAIInterface "Python"

!macro InstallSkirmishAI skirAiName
!ifdef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
  SetOutPath "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
  !ifdef USE_BUILD_DIR
	File /r /x *.a /x *.def /x *.7z /x *.dbg /x CMakeFiles /x Makefile /x cmake_install.cmake "${BUILD_DIR}\AI\Skirmish\${skirAiName}\*.*"
	File /r "..\AI\Skirmish\${skirAiName}\data\*.*"
  !else
	File /r /x *.a /x *.def /x *.7z /x *.dbg "${DIST_DIR}\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
  !endif
  !undef SKIRM_AI_VERS
!endif
!macroend
	${!echonow} "Processing: main: Null Skirmish AIs"
	;TODO: Fix the vc projects to use the same names.
	!insertmacro InstallSkirmishAI "NullAI"
	!insertmacro InstallSkirmishAI "NullOOJavaAI"

	${!echonow} "Processing: main: base archives"
	; Default content
	SetOverWrite on
	SetOutPath "$INSTDIR\base"

	File "${BUILD_OR_DIST_DIR}\base\springcontent.sdz"
	File "${BUILD_OR_DIST_DIR}\base\maphelper.sdz"
	File "${BUILD_OR_DIST_DIR}\base\cursors.sdz"
	SetOutPath "$INSTDIR\base\spring"
	File "${BUILD_OR_DIST_DIR}\base\spring\bitmaps.sdz"

	${!echonow} "Processing: main: demo file association"
	${IfNot} ${FileExists} "$INSTDIR\spring.exe"
		; Demofile file association
		!insertmacro APP_ASSOCIATE "sdf" "spring.demofile" "Spring demo file" \
				"$INSTDIR\spring.exe,0" "Open with Spring" "$\"$INSTDIR\spring.exe$\" $\"%1$\""
		!insertmacro UPDATEFILEASSOC
	${EndIf}

!else
	${!echonow} "Processing: main: Uninstall"

	; Main files
	Delete "$INSTDIR\spring.exe"
	Delete "$INSTDIR\spring.def"
	Delete "$INSTDIR\unitsync.dll"
	Delete "$INSTDIR\PALETTE.PAL"
	Delete "$INSTDIR\selectkeys.txt"
	Delete "$INSTDIR\uikeys.txt"
	Delete "$INSTDIR\cmdcolors.txt"
	Delete "$INSTDIR\ctrlpanel.txt"
	Delete "$INSTDIR\teamcolors.lua"

	; New Settings Program
	Delete "$INSTDIR\springsettings.exe"
	Delete "$INSTDIR\mingwm10.dll"
	Delete "$INSTDIR\wxbase28u_gcc_custom.dll"
	Delete "$INSTDIR\wxbase28u_net_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_adv_gcc_custom.dll"
	Delete "$INSTDIR\wxmsw28u_core_gcc_custom.dll"
	Delete "$INSTDIR\springsettings.conf"

	; DLLs
	Delete "$INSTDIR\DevIL.dll"
	Delete "$INSTDIR\freetype6.dll"
	Delete "$INSTDIR\glew32.dll"
	Delete "$INSTDIR\ILU.dll"
	Delete "$INSTDIR\ILUT.dll"
	Delete "$INSTDIR\python25.dll"
	Delete "$INSTDIR\SDL.dll"
	Delete "$INSTDIR\zlib1.dll"
	Delete "$INSTDIR\OpenAL32.dll"
	; next one is deprecated since mingwlibs 20.1 (around spring v0.81.2.1)
	Delete "$INSTDIR\wrap_oal.dll"
	Delete "$INSTDIR\vorbisfile.dll"
	Delete "$INSTDIR\vorbis.dll"
	Delete "$INSTDIR\ogg.dll"

	Delete "$INSTDIR\libgcc_s_dw2-1.dll"
	Delete "$INSTDIR\MSVCR71.dll"


	Delete "$INSTDIR\PALETTE.PAL"

	; Fonts
	Delete "$INSTDIR\fonts\FreeSansBold.otf"
	RmDir "$INSTDIR\fonts"

!macro DeleteSkirmishAI skirAiName
!ifndef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
  RmDir /r "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
  !undef SKIRM_AI_VERS
!endif
!macroend
	!insertmacro DeleteSkirmishAI "NullAI"
	!insertmacro DeleteSkirmishAI "NullOOJavaAI"
	RmDir "$INSTDIR\AI\Global"
	RmDir "$INSTDIR\AI\Skirmish"

	RmDir "$INSTDIR\AI\Helper-libs"
	RmDir /r "$INSTDIR\AI"

  ; AI Interfaces
!macro DeleteAIInterface aiIntName
!ifndef INSTALL
  ;This is only supported in NSIS 2.39+
  ;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
  ;So we have to use this, which has to be supplied to us on the cmd-line
  !define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
  RmDir /r "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
  !undef AI_INT_VERS
!endif
!macroend
	!insertmacro DeleteAIInterface "C"
	!insertmacro DeleteAIInterface "Java"
	!insertmacro DeleteAIInterface "Python"

	; base content
	Delete "$INSTDIR\base\spring\bitmaps.sdz"
	Delete "$INSTDIR\base\springcontent.sdz"
	Delete "$INSTDIR\base\maphelper.sdz"
	Delete "$INSTDIR\base\cursors.sdz"
	RmDir "$INSTDIR\base\spring"
	RmDir "$INSTDIR\base"

	; Generated stuff from the installer
	Delete "$INSTDIR\${PRODUCT_NAME}.url"
	Delete "$INSTDIR\uninst.exe"

	; Generated stuff from running spring
	Delete "$INSTDIR\ArchiveCacheV7.lua"
	Delete "$INSTDIR\unitsync.log"
	Delete "$INSTDIR\infolog.txt"
	Delete "$INSTDIR\ext.txt"
	RmDir "$INSTDIR\demos"
	RmDir "$INSTDIR\maps"
	RmDir "$INSTDIR\mods"

	; Demofile file association
	!insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"

	MessageBox MB_YESNO|MB_ICONQUESTION "Do you want me to completely remove all spring related files?$\n\
			All maps, mods, screenshots and your settings will be removed. $\n\
			CAREFULL: ALL CONTENTS OF YOUR SPRING INSTALLATION DIRECTORY WILL BE REMOVED!" \
			IDNO skip_purge
		RmDir /r "$INSTDIR"
		Delete "$LOCALAPPDATA\springsettings.cfg"
		Delete "$APPDATA\springlobby.conf"
	skip_purge:

!endif

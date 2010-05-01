
; AI Interfaces
; =============

; Install:
!macro InstallAIInterface aiIntName
	!ifdef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!ifdef AI_INT_VERS_${aiIntName}
			!define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
			!ifdef USE_BUILD_DIR
				${!defineifexist} AI_INT_BUILT "${BUILD_DIR}\AI\Interfaces\${aiIntName}\*.*"
			!else
				${!defineifexist} AI_INT_BUILT "${DIST_DIR}\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
			!endif
			!ifdef AI_INT_BUILT
				SetOutPath "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
				!ifdef USE_BUILD_DIR
					File /r /x *.a /x *.def /x *.7z /x *.dbg /x CMakeFiles /x Makefile /x cmake_install.cmake "${BUILD_DIR}\AI\Interfaces\${aiIntName}\*.*"
					File /r "..\AI\Interfaces\${aiIntName}\data\*.*"
				!else
					File /r /x *.a /x *.def /x *.7z /x *.dbg "${DIST_DIR}\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
				!endif
				!undef AI_INT_BUILT
			!endif
			;buildbot creates 7z, and those get included in installer, fix here until buildserv got fixed
			;File /r "..\AI\Interfaces\${aiIntName}\data\*.*"
			!undef AI_INT_VERS
		!endif
	!endif
!macroend

; Un-Install:
!macro DeleteAIInterface aiIntName
	!ifndef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!ifdef AI_INT_VERS_${aiIntName}
			!define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
			RmDir /r "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
			!undef AI_INT_VERS
		!endif
	!endif
!macroend


; Skirmish AIs
; ============

; Install:
!macro InstallSkirmishAI skirAiName
	!ifdef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!ifdef SKIRM_AI_VERS_${skirAiName}
			!define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
			!ifdef USE_BUILD_DIR
				${!defineifexist} SKIRM_AI_BUILT "${BUILD_DIR}\AI\Skirmish\${skirAiName}\*.*"
			!else
				${!defineifexist} SKIRM_AI_BUILT "${DIST_DIR}\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
			!endif
			!ifdef SKIRM_AI_BUILT
				SetOutPath "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
				!ifdef USE_BUILD_DIR
					File /r /x *.a /x *.def /x *.7z /x *.dbg /x CMakeFiles /x Makefile /x cmake_install.cmake "${BUILD_DIR}\AI\Skirmish\${skirAiName}\*.*"
					File /r "..\AI\Skirmish\${skirAiName}\data\*.*"
				!else
					File /r /x *.a /x *.def /x *.7z /x *.dbg "${DIST_DIR}\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
				!endif
				!undef SKIRM_AI_BUILT
			!endif
			!undef SKIRM_AI_VERS
		!endif
	!endif
!macroend

; Un-Install:
!macro DeleteSkirmishAI skirAiName
	!ifndef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!ifdef SKIRM_AI_VERS_${skirAiName}
			!define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
			RmDir /r "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
			!undef SKIRM_AI_VERS
		!endif
	!endif
!macroend

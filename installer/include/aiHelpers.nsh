
; AI Interfaces
; =============

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

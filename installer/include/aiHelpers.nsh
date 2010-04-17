
!macro InstallAIInterface aiIntName
	!ifdef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file AI_INT_VERS ..\AI\Interfaces\${aiIntName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!define AI_INT_VERS ${AI_INT_VERS_${aiIntName}}
		${If} ${FileExists} "..\game\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
			SetOutPath "$INSTDIR\AI\Interfaces\${aiIntName}\${AI_INT_VERS}"
			File /r /x *.a /x *.def /x *.7z /x *.dbg "..\game\AI\Interfaces\${aiIntName}\${AI_INT_VERS}\*.*"
		${EndIf}
		;buildbot creates 7z, and those get included in installer, fix here until buildserv got fixed
		;File /r "..\AI\Interfaces\${aiIntName}\data\*.*"
		!undef AI_INT_VERS
	!endif
!macroend

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



!macro InstallSkirmishAI skirAiName
	!ifdef INSTALL
		;This is only supported in NSIS 2.39+
		;!define /file SKIRM_AI_VERS ..\AI\Skirmish\${skirAiName}\VERSION
		;So we have to use this, which has to be supplied to us on the cmd-line
		!define SKIRM_AI_VERS ${SKIRM_AI_VERS_${skirAiName}}
		${If} ${FileExists} "..\game\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
			SetOutPath "$INSTDIR\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}"
			File /r /x *.a /x *.def /x *.7z /x *.dbg "..\game\AI\Skirmish\${skirAiName}\${SKIRM_AI_VERS}\*.*"
		${EndIf}
		!undef SKIRM_AI_VERS
	!endif
!macroend

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

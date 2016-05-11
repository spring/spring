;
; Allows toggeling sections by command line switch (mainly for unattened installations)
; all possible parameters are behind ${toggleSection}
; for example:
; 	${toggleSection} "PORTABLE" ${SEC_PORTABLE}
; is the /PORTABLE switch

System::Store "s" ; save all register

!define toggleSection "!insertmacro toggleSection"
!macro toggleSection ParameterName Section
	${getParameterValue} ${ParameterName} "false"
	Pop $0
	${If} $0 == ""
		SectionGetFlags ${Section} $1 ; get current flags
		IntOp $1 $1 & ${SF_SELECTED}
		${If} $1 = ${SF_SELECTED} ; selected?
			IntOp $1 $1 - ${SF_SELECTED} ; unselect
		${Else}
			IntOp $1 $1 | ${SF_SELECTED} ; select
		${Endif}
		SectionSetFlags ${Section} $1 ; set flag for section
	${EndIf}
!macroend

; portable mode
${toggleSection} "PORTABLE" ${SEC_PORTABLE}

; desktop shortcuts
${toggleSection} "NODESKTOPLINK" ${SEC_DESKTOP}

; startmenu
${toggleSection} "NOSTARTMENU" ${SEC_START}

; disable registry writes
${getParameterValue} "NOREGISTRY" "false"
Pop $0

${If} $0 == ""
	IntOp $REGISTRY 0 + 0
${Else}
	IntOp $REGISTRY 0 + 1 ; default, write registry
${EndIf}

System::Store "l" ; restore register


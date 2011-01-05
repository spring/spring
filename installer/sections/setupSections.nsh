;
; Allows toggeling sections by command line switch (mainly for unattened installations)
; all possible parameters are behind ${toggleSection}
; for example:
; 	${toggleSection} "PORTABLE" ${SEC_PORTABLE}
; is the /PORTABLE switch

Push $0 ; save register
Push $1

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

; multithreaded
${toggleSection} "NOMULTITHREAD" ${SEC_GML}

; portable mode
${toggleSection} "PORTABLE" ${SEC_PORTABLE}

; lobbies
${toggleSection} "NOSPRINGLOBBY" ${SEC_SPRINGLOBBY}
${toggleSection} "NOZEROK" ${SEC_ZERO_K_LOBBY}

; server
${toggleSection} "NOTASSERVER" ${SEC_TASSERVER}

; desktop shortcuts
${toggleSection} "NODESKTOPLINK" ${SEC_DESKTOP}

; tools
${toggleSection} "NOARCHIVEMOVER" ${SEC_ARCHIVEMOVER}
${toggleSection} "NORAPID" ${SEC_RAPID}

; startmenu
${toggleSection} "NOSTARTMENU" ${SEC_START}

; ais
${toggleSection} "NOAAI" ${SEC_AAI}
${toggleSection} "NOKAIK" ${SEC_KAIK}
${toggleSection} "NORAI" ${SEC_RAI}
${toggleSection} "NOE323AI" ${SEC_E323AI}

; disable registry writes
${getParameterValue} "NOREGISTRY" "false"
Pop $0

${If} $0 == ""
	IntOp $REGISTRY 0 + 0
${Else}
	IntOp $REGISTRY 0 + 1 ; default, write registry
${EndIf}

Pop $1 ; restore register
Pop $0


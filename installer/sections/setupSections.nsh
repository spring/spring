; Allows disabling of sections by command line switch (for unattened installations)
; current switches:

!define setupSection "!insertmacro setupSection"
!macro setupSection ParameterName Section
	${getParameterValue} ${ParameterName} "false"
	Pop $0
	${If} $0 == ""
		IntOp $0 $0 & ${SECTION_OFF}
		SectionSetFlags ${Section} $0
	${EndIf}
!macroend

; enable portable mode if specified
${getParameterValue} "PORTABLE" "false"
Pop $0
${If} $0 == ""
	SectionSetFlags ${SEC_PORTABLE} 1
${EndIf}

; lobbies
${setupSection} "NOSPRINGLOBBY" ${SEC_SPRINGLOBBY}
${setupSection} "NOZEROK" ${SEC_ZERO_K_LOBBY}

; server
${setupSection} "NOTASSERVER" ${SEC_TASSERVER}

; desktop shortcuts
${setupSection} "NODESKTOPLINK" ${SEC_DESKTOP}

; tools
${setupSection} "NOARCHIVEMOVER" ${SEC_ARCHIVEMOVER}
${setupSection} "NORAPID" ${SEC_RAPID}

; startmenu
${setupSection} "NOSTARTMENU" ${SEC_START}

;ais
${setupSection} "NOAAI" ${SEC_AAI}
${setupSection} "NOKAIK" ${SEC_KAIK}
${setupSection} "NORAI" ${SEC_RAI}
${setupSection} "NOE323AI" ${SEC_E323AI}



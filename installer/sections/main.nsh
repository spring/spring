!ifdef INSTALL

	SetOutPath "$INSTDIR"
	SetOverWrite on

	${If} ${FileExists} "$INSTDIR\uninst.exe"
		MessageBox MB_ICONINFORMATION|MB_YESNO "Spring is already installed in this directory, do you want to uninstall it before continuing?" /SD IDYES IDNO fail
		ExecWait '"$INSTDIR\uninst.exe" /S _?=$INSTDIR' $0
		${If} $0 != 0
			fail:
			Abort "Uninstallation failed, please choose a different installation Directory or cleanup the destination directory."
		${EndIf}
	${EndIf}

	${!echonow} "Processing: engine"

	!insertmacro extractFile "${MIN_PORTABLE_ARCHIVE}" "spring_engine.7z" ""

	CreateDirectory "$INSTDIR\maps"
	CreateDirectory "$INSTDIR\games"

	${!echonow} "Processing: main: demo file association"
	${If} $REGISTRY = 1
		; we don't add here $INSTDIR directly to registry, because file-structure will change in future
		; please use this values directly without modifying them
		WriteRegStr ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngineHelper" "$INSTDIR\unitsync.dll"
		WriteRegStr ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngine" "$INSTDIR\spring.exe"

		; link custom spring:// scheme, so we can start spring from browser
		WriteRegStr HKCR "spring" "" "URL:Spring Protocol"
		WriteRegStr HKCR "spring" "URL Protocol" ""
		WriteRegStr HKCR "spring\DefaultIcon" "" "spring.exe,1"
		WriteRegStr HKCR "spring\shell\open\command" "" "$INSTDIR\spring.exe %1"
	${EndIf}

!else
	${!echonow} "Processing: main: Uninstall"

	; Generated stuff from the installer
	Delete "$INSTDIR\${PRODUCT_NAME}.url"
	Delete "$INSTDIR\uninst.exe"

	; Generated stuff from running spring
	Delete "$INSTDIR\cache\ArchiveCache.lua"
	Delete "$INSTDIR\unitsync.log"
	Delete "$INSTDIR\infolog.txt"
	Delete "$INSTDIR\ext.txt"
	RmDir "$INSTDIR\demos"
	RmDir "$INSTDIR\maps"
	RmDir "$INSTDIR\games"

	; Registry Keys
	DeleteRegKey ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY}
	DeleteRegKey HKCR "spring"

	MessageBox MB_YESNO|MB_ICONQUESTION "Do you want me to completely remove all spring related files?$\n\
			All maps, games, screenshots and your settings will be removed. $\n\
			CAREFULL: ALL CONTENTS OF YOUR SPRING INSTALLATION DIRECTORY WILL BE REMOVED! " \
			/SD IDNO IDNO skip_purge
		RmDir /r "$INSTDIR"
		RmDir /r "$DOCUMENTS\My Games\Spring"
		Delete "$LOCALAPPDATA\springsettings.cfg"
	skip_purge:

!endif

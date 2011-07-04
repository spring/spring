!ifdef INSTALL

	SetOutPath "$INSTDIR"
	SetOverWrite on

	${!echonow} "Processing: engine"

	!insertmacro extractFile "${MIN_PORTABLE_ARCHIVE}" "spring_engine.7z" ""

	; archive is portable, make it normal
	Delete "$INSTDIR\springsettings.cfg"

	CreateDirectory "$INSTDIR\maps"
	CreateDirectory "$INSTDIR\games"

	${!echonow} "Processing: main: demo file association"
	${If} $REGISTRY = 1
		${If} ${FileExists} "$INSTDIR\spring.exe"
			; Demofile file association
			!insertmacro APP_ASSOCIATE "sdf" "spring.demofile" "Spring demo file" \
				"$INSTDIR\spring.exe,0" "Open with Spring" "$\"$INSTDIR\spring.exe$\" $\"%1$\""
			!insertmacro UPDATEFILEASSOC
			; we don't add here $INSTDIR directly to registry, because file-structure will change in future
			; please use this values directly without modifying them
			WriteRegStr ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngineHelper" "$INSTDIR\unitsync.dll"
			WriteRegStr ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngine" "$INSTDIR\spring.exe"
		${EndIf}
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
	DeleteRegValue ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngineHelper"
	DeleteRegValue ${PRODUCT_ROOT_KEY} ${PRODUCT_KEY} "SpringEngine"

	; Demofile file association
	!insertmacro APP_UNASSOCIATE "sdf" "spring.demofile"

	MessageBox MB_YESNO|MB_ICONQUESTION "Do you want me to completely remove all spring related files?$\n\
			All maps, games, screenshots and your settings will be removed. $\n\
			CAREFULL: ALL CONTENTS OF YOUR SPRING INSTALLATION DIRECTORY WILL BE REMOVED! " \
			/SD IDNO IDNO skip_purge
		RmDir /r "$INSTDIR"
		Delete "$LOCALAPPDATA\springsettings.cfg"
		Delete "$APPDATA\springlobby.conf"
	skip_purge:

!endif

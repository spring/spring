!ifdef INSTALL
	SetOutPath "$INSTDIR"

	!ifdef USE_BUILD_DIR 
		File "${BUILD_DIR}\tools\ArchiveMover\ArchiveMover.exe"
	!else
		File "${DIST_DIR}\ArchiveMover.exe"
	!endif

	; Archive file associations
	${If} $REGISTRY = 1
		!insertmacro APP_ASSOCIATE "sd7" "spring.sd7_archive" "Spring content package" \
			"$INSTDIR\ArchiveMover.exe,0" "Open with Spring" \
			"$\"$INSTDIR\ArchiveMover.exe$\" $\"%1$\""
		!insertmacro APP_ASSOCIATE "sdz" "spring.sdz_archive" "Spring content package" \
			"$INSTDIR\ArchiveMover.exe,0" "Open with Spring" \
			"$\"$INSTDIR\ArchiveMover.exe$\" $\"%1$\""

		!insertmacro UPDATEFILEASSOC
	${EndIf}

!else

	; Archive file associations
	!insertmacro APP_UNASSOCIATE "sd7" "spring.sd7_archive"
	!insertmacro APP_UNASSOCIATE "sdz" "spring.sdz_archive"

	Delete "$INSTDIR\ArchiveMover.exe"

!endif ; !INSTALL

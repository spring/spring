!ifdef INSTALL
	SetOutPath "$INSTDIR"

	!ifdef ARCHIVEMOVER
		!insertmacro extractFile "${ARCHIVEMOVER}" "archivemover.7z" ""
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
	!endif
!else

	; Archive file associations
	!insertmacro APP_UNASSOCIATE "sd7" "spring.sd7_archive"
	!insertmacro APP_UNASSOCIATE "sdz" "spring.sdz_archive"

	Delete "$INSTDIR\ArchiveMover.exe"

!endif ; !INSTALL

### builds
#
# Macros and fucntions defined in this file:
# * CreateEngineBuildInstallTarget
#


# Only install the given engine target executable & dependencies
# example:
# 	CreateEngineBuildInstallTarget(spring-headless)
# ->
# 	make install-spring-headless
macro    (CreateEngineBuildInstallTarget targetName)
	# relative source path, eg: "rts/builds/default"
	file(RELATIVE_PATH relSrcDirPath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	# Only install spring-<targetName> executable & dependencies
	set(${targetName}-InstallDirs
			"${relSrcDirPath}"
			"tools/unitsync"
			"cont"
			"AI"
		)
	set(${targetName}-InstallDeps
			${targetName}
			gamedata
			unitsync
			${DEPS_AI_ALL}
			ArchiveMover
		)
	CreateInstallTarget(${targetName} ${targetName}-InstallDeps ${targetName}-InstallDirs)
endmacro (CreateEngineBuildInstallTarget targetName)


if    (NOT HEADLESS_SYSTEM)
	Add_Subdirectory(default)
	Add_Subdirectory(GML)
endif (NOT HEADLESS_SYSTEM)

Add_Subdirectory(DS)
Add_Subdirectory(HL)

### builds
#
# Macros and fucntions defined in this file:
# * CreateEngineBuildAndInstallTarget
#


# Only install the given engine target executable & dependencies
# example:
# 	CreateEngineBuildAndInstallTarget(headless)
# ->
# 	make spring-headless
# 	make install-spring-headless
macro    (CreateEngineBuildAndInstallTarget targetName)
	# relative source path, eg: "rts/builds/default"
	file(RELATIVE_PATH relSrcDirPath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	set(myBuildTarget spring-${targetName})
	set(${targetName}-Deps
			engine-${targetName}
			gamedata
			unitsync
			${DEPS_AI_ALL}
			ArchiveMover
		)
	# Create a custom meta build target
	add_custom_target(${myBuildTarget}
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		COMMENT
			"  ${myBuildTarget}: Building all dependencies ..." VERBATIM
		)
	# This also works for custom targets
	add_dependencies(${myBuildTarget} ${${targetName}-Deps})

	# Only install spring-<targetName> executable & dependencies
	set(${targetName}-InstallDirs
			"${relSrcDirPath}"
			"tools/unitsync"
			"cont"
			"AI"
		)
	CreateInstallTarget(spring-${targetName} myBuildTarget ${targetName}-InstallDirs)
endmacro (CreateEngineBuildAndInstallTarget targetName)


if    (NOT HEADLESS_SYSTEM)
	Add_Subdirectory(default)
	Add_Subdirectory(GML)
endif (NOT HEADLESS_SYSTEM)

Add_Subdirectory(DS)
Add_Subdirectory(HL)

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
			manpages
			userdocs
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
			"doc"
			"tools/unitsync"
			"cont"
		)
	If    (NOT "${AI_TYPES}" STREQUAL "NONE")
		Set(${targetName}-InstallDirs ${${targetName}-InstallDirs} "AI")
	EndIf (NOT "${AI_TYPES}" STREQUAL "NONE")
	CreateInstallTarget(spring-${targetName} myBuildTarget ${targetName}-InstallDirs)
endmacro (CreateEngineBuildAndInstallTarget targetName)


# This allows us to disable each build type individually at configure stage.
Macro    (AddEngineBuild build)
	Set(CONFIGURE_BUILD_${build} TRUE CACHE BOOL "Build the ${build} build-type of the engine.")
	If     (CONFIGURE_BUILD_${build})
		Add_Subdirectory(${build})
	EndIf  ()
EndMacro (AddEngineBuild)

if    (NOT HEADLESS_SYSTEM)
	AddEngineBuild(default)
	AddEngineBuild(multithreaded)
endif (NOT HEADLESS_SYSTEM)

AddEngineBuild(dedicated)
AddEngineBuild(headless)

### builds
#
# Macros and fucntions defined in this file:
# * CreateEngineBuildAndInstallTarget
#

Include_Directories(${CMAKE_BINARY_DIR}/src-generated/engine)

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
	Add_Dependencies(engine-${targetName} generateVersionFiles basecontent)

	# Only install spring-<targetName> executable & dependencies
	set(${targetName}-InstallDirs
			"${relSrcDirPath}"
			"doc"
			"tools/unitsync"
			"cont"
			"rts/lib/luasocket"
		)
	If    (NOT "${AI_TYPES}" STREQUAL "NONE")
		Set(${targetName}-InstallDirs ${${targetName}-InstallDirs} "AI")
	EndIf (NOT "${AI_TYPES}" STREQUAL "NONE")

	CreateInstallTarget(spring-${targetName} myBuildTarget ${targetName}-InstallDirs)

	# allow easy switching of the default engine
	if    (${targetName} STREQUAL "multithreaded")
		# Create shortcuts
		# * make spring
		# * make install-spring
		message(STATUS "Using ${targetName} as default engine")
		add_dependencies(spring spring-${targetName}) # This also works for custom targets
		add_dependencies(install-spring install-spring-${targetName}) # This also works for custom targets
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring")
	else()
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring-${targetName}")
	endif ()
endmacro (CreateEngineBuildAndInstallTarget targetName)


# This allows us to disable each build type individually at configure stage.
Macro    (AddEngineBuild build)
	option(BUILD_spring-${build} "Configure the spring-${build} target." TRUE)
	If     (BUILD_spring-${build})
		Add_Subdirectory(${build})
	EndIf  ()
EndMacro (AddEngineBuild)

# add default dummy target which is used later on
add_custom_target(spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_custom_target(install-spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

if    (NOT HEADLESS_SYSTEM)
	### Add icon and manifest to exe using windres
	IF    (WIN32 AND MINGW)
		SET(ENGINE_ICON "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	ENDIF (WIN32)
	AddEngineBuild(multithreaded) ## ASIM GML OMP
	AddEngineBuild(legacy) ## OMP
endif (NOT HEADLESS_SYSTEM)

AddEngineBuild(dedicated)
AddEngineBuild(headless)


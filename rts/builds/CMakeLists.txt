### builds
#
# Macros and functions defined in this file:
# * create_engine_build_and_install_target
#


# Only install the given engine target executable & dependencies
# example:
# 	create_engine_build_and_install_target(headless)
# ->
# 	make spring-headless
# 	make install-spring-headless
macro    (create_engine_build_and_install_target targetName)
	# relative source path, eg: "rts/builds/default"
	file(RELATIVE_PATH relSrcDirPath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	set(myBuildTarget spring-${targetName})
	set(${targetName}-Deps
			engine-${targetName}
			basecontent
			unitsync
			${DEPS_AI_ALL}
		)
	if (CREATE_MAN_PAGES)
		list(APPEND ${targetName}-Deps manpages)
	endif ()
	if (userdoc_INSTALL_HTML)
		list(APPEND ${targetName}-Deps userdocs)
	endif ()

	# Create a custom meta build target
	add_custom_target(${myBuildTarget}
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		COMMENT
			"  ${myBuildTarget}: Building all dependencies ..." VERBATIM
		)
	# This also works for custom targets
	add_dependencies(${myBuildTarget} ${${targetName}-Deps})
	add_dependencies(engine-${targetName} generateVersionFiles basecontent)

	# Only install spring-<targetName> executable & dependencies
	set(${targetName}-InstallDirs
			"${relSrcDirPath}"
			"doc"
			"tools/unitsync"
			"cont"
			"rts/lib/luasocket"
		)
	if    (NOT "${AI_TYPES}" STREQUAL "NONE")
		set(${targetName}-InstallDirs ${${targetName}-InstallDirs} "AI")
	endif ()

	create_install_target(spring-${targetName} myBuildTarget ${targetName}-InstallDirs)

	# allow easy switching of the default engine
	if    (${targetName} STREQUAL "opengl4")
		# Create shortcuts
		# * make spring
		# * make install-spring
		message(STATUS "Using ${targetName} as default engine")
		add_dependencies(spring spring-${targetName}) # This also works for custom targets
		add_dependencies(install-spring install-spring-${targetName}) # This also works for custom targets
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring")
	else ()
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring-${targetName}")
	endif ()
	target_include_directories(engine-${targetName} PRIVATE
		${gflags_BINARY_DIR}/include
		${CMAKE_BINARY_DIR}/src-generated/engine
	)
	target_link_libraries(engine-${targetName}
		buildtype_${targetName}
	)
endmacro ()


# This allows us to disable each build type individually at configure stage.
macro    (add_engine_build build)
	option(BUILD_spring-${build} "Configure the spring-${build} target." TRUE)
	if     (BUILD_spring-${build})
		add_subdirectory(${build})
	endif ()
endmacro ()

# add default dummy target which is used later on
add_custom_target(spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_custom_target(install-spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

### Add icon and manifest to exe using windres
if    (WIN32)
	set(ENGINE_ICON "${ENGINE_SRC_ROOT_DIR}/icon.rc")

	if (MSVC)
		# Make sure we disable the default manifest generation to prevent conflicts
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
	endif ()
endif ()

# the build targets
add_engine_build(opengl4)
add_engine_build(dedicated)
add_engine_build(headless)

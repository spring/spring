### C & C++ AI Interface
#
# Global variables set in this file:
# * BUILD_C_AIINTERFACE
# * C_AIINTERFACE_VERS
# * C_AIINTERFACE_TARGET
#


################################################################################
### BEGINN: MACROS_AND_FUNCTIONS
# Define macros and functions to be used in this file and by Skirmish AIs

# Native Skirmish AI configuration macro
# This will be called from native AIs at AI/Skirmish/*/CMakeLists.txt.
macro    (configure_native_skirmish_ai mySourceDirRel_var additionalSources_var additionalCompileFlags_var additionalLibraries_var)
	set(myDir                  "${CMAKE_CURRENT_SOURCE_DIR}")
	get_last_path_part(dirName ${myDir})
	set(myName                 "${dirName}")
	set(mySourceDirRel         "${${mySourceDirRel_var}}") # Common values are "" or "src"
	set(additionalSources      ${${additionalSources_var}})
	set(additionalCompileFlags ${${additionalCompileFlags_var}})
	set(additionalLibraries    ${${additionalLibraries_var}})

	# Check if the user wants to compile the AI
	if    (BUILD_C_AIINTERFACE AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
		set(BUILD_THIS_SKIRMISHAI TRUE)
	else  (BUILD_C_AIINTERFACE AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
		set(BUILD_THIS_SKIRMISHAI FALSE)
		message("warning: ${myName} Skirmish AI will not be built!")
	endif (BUILD_C_AIINTERFACE AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")

	# Compile and install
	if    (BUILD_THIS_SKIRMISHAI)
		# Assemble meta data
		get_version_plus_dep_file(myVersion myVersionDepFile)
		set(myTarget      "${myName}")
		set(myInstLibsDir "${SKIRMISH_AI_LIBS}/${myName}/${myVersion}")
		set(myInstDataDir "${SKIRMISH_AI_DATA}/${myName}/${myVersion}")
		make_absolute(mySourceDir "${myDir}" "${mySourceDirRel}")

		skirmish_ai_message(STATUS "Found Skirmish AI: ${myName} ${myVersion}")

		# Create a list of all the AIs own source files
		get_native_sources_recursive(mySources "${mySourceDir}" "${myDir}")

		# Compile the library
		add_library(${myTarget} MODULE ${mySources} ${additionalSources} ${myVersionDepFile})
		set_target_properties(${myTarget} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
		fix_lib_name(${myTarget})

		add_dependencies(${myTarget} generateVersionFiles)
		target_link_libraries(${myTarget} ${additionalLibraries})
		set_target_properties(${myTarget} PROPERTIES COMPILE_FLAGS "${additionalCompileFlags}")
		set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME   "SkirmishAI")

		# Install the data files
		install(DIRECTORY "${myDir}/data/" DESTINATION "${myInstLibsDir}" FILES_MATCHING PATTERN REGEX "AI(Info|Options)\\.lua$")
		install(DIRECTORY "${myDir}/data/" DESTINATION "${myInstDataDir}" FILES_MATCHING PATTERN REGEX "AI(Info|Options)\\.lua$" EXCLUDE PATTERN "*")
		# Install the library
		install(TARGETS   ${myTarget}      DESTINATION "${myInstLibsDir}")
	endif (BUILD_THIS_SKIRMISHAI)
endmacro (configure_native_skirmish_ai mySourceDirRel_var additionalSources_var additionalCompileFlags_var additionalLibraries_var)

### END: MACROS_AND_FUNCTIONS
################################################################################


set(myName            "C")
set(myDir             "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDirRel    "src")


# Check if the user wants to compile the interface
if    ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AI_TYPES_NATIVE TRUE)
else  ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AI_TYPES_NATIVE FALSE)
endif ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")

if    (AI_TYPES_NATIVE AND myName MATCHES "${AI_EXCLUDE_REGEX}")
	set(AI_TYPES_NATIVE FALSE)
endif (AI_TYPES_NATIVE AND myName MATCHES "${AI_EXCLUDE_REGEX}")


# Check dependencies of the interface are met
if    (AI_TYPES_NATIVE)
	set_global(BUILD_${myName}_AIINTERFACE TRUE)
else  (AI_TYPES_NATIVE)
	set_global(BUILD_${myName}_AIINTERFACE FALSE)
	message ("warning: C & C++ AI Interface will not be built!")
endif (AI_TYPES_NATIVE)


# Build
if    (BUILD_${myName}_AIINTERFACE)
	get_version_plus_dep_file(myVersion myVersionDepFile)
	set(myTarget      "${myName}-AIInterface")
	set(myInstLibsDir ${AI_INTERFACES_LIBS}/${myName}/${myVersion})
	set(myInstDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})
	make_absolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	ai_interface_message(STATUS "Found AI Interface: ${myTarget} ${myVersion}")

	set_global(${myName}_AIINTERFACE_VERS   ${myVersion})
	set_global(${myName}_AIINTERFACE_TARGET ${myTarget})

	set(mySources
			"${mySourceDir}/Interface.cpp"
			"${mySourceDir}/InterfaceExport.cpp"
			"${mySourceDir}/SSkirmishAISpecifier.cpp"
		)
	add_library(${myTarget} MODULE ${mySources} ${ai_common_SRC} ${myVersionDepFile})
	set_target_properties(${myTarget} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
	target_link_libraries(${myTarget} CUtils)
	fix_lib_name(${myTarget})

	add_dependencies(${myTarget} generateVersionFiles)
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "AIInterface")

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myInstLibsDir}" FILES_MATCHING PATTERN REGEX "InterfaceInfo\\.lua$")
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myInstDataDir}" FILES_MATCHING PATTERN REGEX "InterfaceInfo\\.lua$" EXCLUDE PATTERN "*")
	# Install the library
	install(TARGETS   ${myTarget}      DESTINATION "${myInstLibsDir}")
endif (BUILD_${myName}_AIINTERFACE)

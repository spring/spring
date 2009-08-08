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
macro    (ConfigureNativeSkirmishAI mySourceDirRel_var additionalSources_var additionalCompileFlags_var additionalLibraries_var)
	set(myDir                  "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName                 "${dirName}")
	set(mySourceDirRel         "${${mySourceDirRel_var}}") # Common values are "" or "src"
	set(additionalSources      ${${additionalSources_var}})
	set(additionalCompileFlags ${${additionalCompileFlags_var}})
	set(additionalLibraries    ${${additionalLibraries_var}})

	# Check if the user wants to compile the AI
	if    (BUILD_C_AIINTERFACE)
		set(BUILD_THIS_SKIRMISHAI TRUE)
	else  (BUILD_C_AIINTERFACE)
		set(BUILD_THIS_SKIRMISHAI FALSE)
		message("warning: ${myName} Skirmish AI will not be built!")
	endif (BUILD_C_AIINTERFACE)

	# Compile and install
	if    (BUILD_THIS_SKIRMISHAI)
		# Assemble meta data
		GetVersionFromFile(myVersion "${myDir}/VERSION")
		set(myTarget    "${myName}")
		set(myDataDir   "${SKIRMISH_DATA}/${myName}/${myVersion}")
		MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

		SkirmishAIMessage(STATUS "Found Skirmish AI: ${myName} ${myVersion}")

		# Create a list of all the AIs own source files
		GetNativeSourcesRecursive(mySources "${mySourceDir}" "${myDir}")

		# Compile the library
		add_library(${myTarget} MODULE ${mySources} ${additionalSources})
		target_link_libraries(${myTarget} ${additionalLibraries})
		set_target_properties(${myTarget} PROPERTIES COMPILE_FLAGS "${additionalCompileFlags}")
		set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME   "SkirmishAI")

		# Install the data files
		install(DIRECTORY "${myDir}/data/" DESTINATION "${myDataDir}")
		# Install the library
		install(TARGETS   ${myTarget}      DESTINATION "${myDataDir}")
	endif (BUILD_THIS_SKIRMISHAI)
endmacro (ConfigureNativeSkirmishAI mySourceDirRel_var additionalSources_var additionalCompileFlags_var additionalLibraries_var)

### END: MACROS_AND_FUNCTIONS
################################################################################


set(myName            "C")
set(myDir             "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDirRel    "src")


# Check if the user wants to compile the interface
if    ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE TRUE)
else  ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE FALSE)
endif ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")


# Check dependencies of the interface are met
if    (AIINTERFACES_NATIVE)
	SetGlobal(BUILD_${myName}_AIINTERFACE TRUE)
else  (AIINTERFACES_NATIVE)
	SetGlobal(BUILD_${myName}_AIINTERFACE FALSE)
	message ("warning: C & C++ AI Interface will not be built!")
endif (AIINTERFACES_NATIVE)


# Build
if    (BUILD_${myName}_AIINTERFACE)
	GetVersionFromFile(myVersion ${myDir}/VERSION)
	set(myTarget  "${myName}-AIInterface")
	set(myDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIInterfaceMessage(STATUS "Found AI Interface: ${myTarget} ${myVersion}")

	SetGlobal(${myName}_AIINTERFACE_VERS   ${myVersion})
	SetGlobal(${myName}_AIINTERFACE_TARGET ${myTarget})

	aux_source_directory(${mySourceDir} mySources)
	list(APPEND mySources ${rts}/System/Platform/SharedLib)
	if    (UNIX)
		list (APPEND mySources ${rts}/System/Platform/Linux/SoLib)
	else  (UNIX)
		list (APPEND mySources ${rts}/System/Platform/Win/DllLib)
	endif (UNIX)
	add_library(${myTarget} MODULE ${mySources} ${ai_common_SRC} ${CUtils_SRC})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "AIInterface")

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myDataDir}")
	# Install the library
	install(TARGETS   ${myTarget}      DESTINATION "${myDataDir}")
endif (BUILD_${myName}_AIINTERFACE)

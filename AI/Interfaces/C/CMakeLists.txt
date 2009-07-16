### C & C++ AI Interface
#
# Global variables set in this file:
# * BUILD_C_AIINTERFACE
# * C_AIINTERFACE_VERS
# * C_AIINTERFACE_TARGET
#

set(myName            "C")
set(myDir             "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDir       "${myDir}/src")


# Check if the user wants to compile the interface
if    ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE TRUE)
else  ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE FALSE)
endif ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")

set(myDir "${CMAKE_CURRENT_SOURCE_DIR}")

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
	set(myTarget  "${myName}-AI-Interface")
	set(myDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})

	AIInterfaceMessage(STATUS "Found ${myTarget} ${myVersion}")

	SetGlobal(${myName}_AIINTERFACE_VERS ${myVersion})
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

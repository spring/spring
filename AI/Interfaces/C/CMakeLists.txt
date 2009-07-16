### C & C++ AI Interface
#
# BUILD_${myName}_AIINTERFACE is set in this file (PARENT_SCOPE),
# and used in ../../Skirmish/*/CMakeLists.txt
#

set(myName            "C")
set(myDir             "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDir       "${myDir}")


# Check if the user wants to compile the interface
if    ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE TRUE)
else  ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")
	set(AIINTERFACES_NATIVE FALSE)
endif ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "NATIVE")

set(myDir "${CMAKE_CURRENT_SOURCE_DIR}")

# Check dependencies of the interface are met
if    (AIINTERFACES_NATIVE)
	set(BUILD_${myName}_AIINTERFACE TRUE)
else  (AIINTERFACES_NATIVE)
	set(BUILD_${myName}_AIINTERFACE FALSE)
	message ("warning: C & C++ AI Interface will not be built!")
endif (AIINTERFACES_NATIVE)
set(BUILD_${myName}_AIINTERFACE ${BUILD_${myName}_AIINTERFACE} PARENT_SCOPE)

# Build
if    (BUILD_${myName}_AIINTERFACE)
	GetVersionFromFile(myVersion ${myDir}/VERSION)
	#set(${myName}_AIINTERFACE_VERS ${myVersion} PARENT_SCOPE)
	# The target has to be set in ../../Skirmish/CMakeLists.txt as well,
	# as compilation fails otherwise with multiple threads;
	# using PARENT_SCOPE seems not to work.
	set(myTarget "${myName}-AI-Interface")
	#set(${myName}_AIINTERFACE_TARGET ${myTarget} PARENT_SCOPE)
	set(myDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})
	AIInterfaceMessage(STATUS "Found: ${myTarget} ${myVersion}")

	aux_source_directory(${myDir} mySources)
	list(APPEND mySources ${CMAKE_SOURCE_DIR}/rts/System/Platform/SharedLib)
	if    (UNIX)
		list (APPEND mySources ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/SoLib)
	else  (UNIX)
		list (APPEND mySources ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/DllLib)
	endif (UNIX)
	add_library(${myTarget} MODULE ${mySources} ${ai_common_SRC} ${CUtils_SRC})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "AIInterface")

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myDataDir}")
	# Install the library
	install(TARGETS   ${myTarget}      DESTINATION "${myDataDir}")
endif (BUILD_${myName}_AIINTERFACE)

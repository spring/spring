### Legacy C++ AI Wrapper
#
# Global variables set in this file:
# * BUILD_LegacyCpp_AIWRAPPER
# * LegacyCpp_AIWRAPPER_TARGET
# * LegacyCpp_Creg_AIWRAPPER_TARGET
#


set(mySourceDirRel    "")


# Check if the user wants to compile the wrapper
if    ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AIWRAPPERS_NATIVE TRUE)
else  ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AIWRAPPERS_NATIVE FALSE)
endif ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")


# Check dependencies of the wrapper are met
if    (AIWRAPPERS_NATIVE)
	SetGlobal(BUILD_${myName}_AIWRAPPER TRUE)
else  (AIWRAPPERS_NATIVE)
	SetGlobal(BUILD_${myName}_AIWRAPPER FALSE)
	message ("warning: Legacy C++ AI Wrapper will not be built!")
endif (AIWRAPPERS_NATIVE)


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	set(myCregTarget "${myName}-Creg-AIWrapper")
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIWrapperMessage(STATUS "Found AI Wrapper: ${myTarget}")
	AIWrapperMessage(STATUS "Found AI Wrapper: ${myCregTarget}")

	SetGlobal(${myName}_AIWRAPPER_TARGET      ${myTarget})
	SetGlobal(${myName}_Creg_AIWRAPPER_TARGET ${myCregTarget})

	# Assemble the Legacy C++ AI sources
	aux_source_directory("${mySourceDir}" mySources)
	list(APPEND mySources ${ai_common_SRC})
	list(APPEND mySources ${rts}/System/float3)
	list(APPEND mySources ${rts}/Sim/Misc/DamageArray)

	# Compile the Legacy C++ AI wrapper static library
	add_library(${myTarget}     STATIC ${mySources})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "${myName}")

	# Compile the Legacy C++ AI wrapper static library with creg support
	add_library(${myCregTarget} STATIC ${mySources} ${creg_SRC})
	set_target_properties(${myCregTarget} PROPERTIES OUTPUT_NAME   "${myName}Creg")
	set_target_properties(${myCregTarget} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")
endif (BUILD_${myName}_AIWRAPPER)

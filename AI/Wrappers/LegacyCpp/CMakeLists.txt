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
	set_global(BUILD_${myName}_AIWRAPPER TRUE)
else  (AIWRAPPERS_NATIVE)
	set_global(BUILD_${myName}_AIWRAPPER FALSE)
	message ("warning: Legacy C++ AI Wrapper will not be built!")
endif (AIWRAPPERS_NATIVE)


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	get_last_path_part(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	set(myCregTarget "${myName}-Creg-AIWrapper")
	make_absolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	ai_wrapper_message(STATUS "Found AI Wrapper: ${myTarget}")
	ai_wrapper_message(STATUS "Found AI Wrapper: ${myCregTarget}")

	set_global(${myName}_AIWRAPPER_TARGET      ${myTarget})
	set_global(${myName}_Creg_AIWRAPPER_TARGET ${myCregTarget})

	# Assemble the Legacy C++ AI sources
	set(mySources
		"${mySourceDir}/AIAI.cpp"
		"${mySourceDir}/AIAICallback.cpp"
		"${mySourceDir}/AIAICheats.cpp"
		"${mySourceDir}/AIGlobalAICallback.cpp"
		"${mySourceDir}/DamageArray.cpp"
		"${mySourceDir}/MoveData.cpp"
		"${mySourceDir}/UnitDef.cpp"
		"${rts}/ExternalAI/AISCommands.cpp"
		"${rts}/Sim/Units/CommandAI/Command.cpp"
		"${rts}/System/float3.cpp"
		${ai_common_SRC}
		)

	# Compile the Legacy C++ AI wrapper static library
	add_library(${myTarget}     STATIC ${mySources})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "${myName}")
	add_dependencies(${myTarget} generateVersionFiles)

	# Compile the Legacy C++ AI wrapper static library with creg support
	add_library(${myCregTarget} STATIC ${mySources} ${sources_engine_System_creg})
	add_dependencies(${myCregTarget} generateVersionFiles)
	set_target_properties(${myCregTarget} PROPERTIES OUTPUT_NAME   "${myName}Creg")
	set_target_properties(${myCregTarget} PROPERTIES COMPILE_FLAGS "-DUSING_CREG")
endif (BUILD_${myName}_AIWRAPPER)

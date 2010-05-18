### (New) C++ AI Wrapper
#
# Global variables set in this file:
# * BUILD_Cpp_AIWRAPPER
# * Cpp_AIWRAPPER_TARGET
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
	message ("warning: (New) C++ AI Wrapper will not be built!")
endif (AIWRAPPERS_NATIVE)


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIWrapperMessage(STATUS "Found AI Wrapper: ${myTarget}")

	SetGlobal(${myName}_AIWRAPPER_TARGET      ${myTarget})


	# Build static library
	if    (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "${MINGWLIBS}/bin/awk.exe")
	else  (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "awk")
	endif (CMAKE_HOST_WIN32)
	set(myBinDir                   "${myDir}/bin")
	set(commonBinDir               "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myGeneratedSourceDir       "${CMAKE_CURRENT_BINARY_DIR}/src-generated")
	set(springSourceDir            "${PROJECT_SOURCE_DIR}")
	set(springAIInterfaceSourceDir "${springSourceDir}/rts/ExternalAI/Interface")

	SetGlobal(${myName}_AIWRAPPER_GENERATED_SRC_DIR "${myGeneratedSourceDir}")
	
	set(myGeneratedSources
		${myGeneratedSourceDir}/AICallback.cpp
		${myGeneratedSourceDir}/Camera.cpp
		${myGeneratedSourceDir}/Cheats.cpp
		${myGeneratedSourceDir}/ClbGroupSupportedCommandImpl.cpp
		${myGeneratedSourceDir}/ClbUnitSupportedCommandImpl.cpp
		${myGeneratedSourceDir}/CurrentCommand.cpp
		${myGeneratedSourceDir}/Damage.cpp
		${myGeneratedSourceDir}/DataDirs.cpp
		${myGeneratedSourceDir}/Debug.cpp
		${myGeneratedSourceDir}/Drawer.cpp
		${myGeneratedSourceDir}/Economy.cpp
		${myGeneratedSourceDir}/Engine.cpp
		${myGeneratedSourceDir}/Feature.cpp
		${myGeneratedSourceDir}/FeatureDef.cpp
		${myGeneratedSourceDir}/FlankingBonus.cpp
		${myGeneratedSourceDir}/Game.cpp
		${myGeneratedSourceDir}/Group.cpp
		${myGeneratedSourceDir}/Gui.cpp
		${myGeneratedSourceDir}/Info.cpp
		${myGeneratedSourceDir}/Line.cpp
		${myGeneratedSourceDir}/Log.cpp
		${myGeneratedSourceDir}/Map.cpp
		${myGeneratedSourceDir}/Mod.cpp
		${myGeneratedSourceDir}/ModParam.cpp
		${myGeneratedSourceDir}/MoveData.cpp
		${myGeneratedSourceDir}/OptionValues.cpp
		${myGeneratedSourceDir}/OrderPreview.cpp
		${myGeneratedSourceDir}/Point.cpp
		${myGeneratedSourceDir}/Resource.cpp
		${myGeneratedSourceDir}/Roots.cpp
		${myGeneratedSourceDir}/Shield.cpp
		${myGeneratedSourceDir}/SkirmishAI.cpp
		${myGeneratedSourceDir}/SkirmishAIs.cpp
		${myGeneratedSourceDir}/Teams.cpp
		${myGeneratedSourceDir}/Unit.cpp
		${myGeneratedSourceDir}/UnitDef.cpp
		${myGeneratedSourceDir}/Version.cpp
		${myGeneratedSourceDir}/WeaponDef.cpp
		${myGeneratedSourceDir}/WeaponMount.cpp)

	file(MAKE_DIRECTORY "${myGeneratedSourceDir}")

	# generate the source files
	SET(myGenerateCommand
		"${AWK_COMMAND}"
		"-v" "SPRING_SOURCE_DIR=${springSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myGeneratedSourceDir}"
		"-f" "${myBinDir}/wrappCallback.awk"
		"-f" "${commonBinDir}/common.awk"
		"-f" "${commonBinDir}/commonDoc.awk"
		"-f" "${commonBinDir}/commonOOCallback.awk"
		"${springAIInterfaceSourceDir}/SSkirmishAICallback.h")

	# this will regenerate the sources whenever the DEPENDS changes
	ADD_CUSTOM_COMMAND(
		DEPENDS "${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
		OUTPUT ${myGeneratedSources}
		COMMAND ${myGenerateCommand}
		WORKING_DIRECTORY "${myBinDir}"
		COMMENT "  Generating C++ AI Wrapper Callback source files" VERBATIM)

	# build the static lib
	set(mySources ${myGeneratedSources})
	list (APPEND mySources ${ai_common})
	add_library(${myTarget} STATIC ${mySources})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "${myName}")
	set_source_files_properties(${myGeneratedSources} PROPERTIES GENERATED TRUE)
endif (BUILD_${myName}_AIWRAPPER)

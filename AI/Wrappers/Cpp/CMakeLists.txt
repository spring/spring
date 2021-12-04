### (New) C++ AI Wrapper
#
# Global variables set in this file:
# * BUILD_Cpp_AIWRAPPER
# * Cpp_AIWRAPPER_TARGET
#


set(myName         "Cpp")
set(mySourceDirRel "src")


# Check if the user wants to compile the wrapper
if    ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AIWRAPPERS_NATIVE TRUE)
else  ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")
	set(AIWRAPPERS_NATIVE FALSE)
endif ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "NATIVE")


# Check dependencies of the wrapper are met
if    (AIWRAPPERS_NATIVE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIWRAPPER TRUE)
else  (AIWRAPPERS_NATIVE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIWRAPPER FALSE)
	message ("warning: (New) C++ AI Wrapper will not be built!")
endif (AIWRAPPERS_NATIVE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	set(myGenTarget  "${myTarget}-generateSources")
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIWrapperMessage(STATUS "Found AI Wrapper: ${myTarget}")

	SetGlobal(${myName}_AIWRAPPER_TARGET      ${myTarget})


	# Build static library
	set(myAwkScriptsDir            "${myDir}/bin")
	set(commonAwkScriptsDir        "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myGeneratedSourceDir       "${CMAKE_CURRENT_BINARY_DIR}/src-generated")
	set(springSourceDir            "${PROJECT_SOURCE_DIR}")
	set(springAIInterfaceSourceDir "${springSourceDir}/rts/ExternalAI/Interface")

	set(${myName}_AIWRAPPER_INCLUDE_DIRS "${mySourceDir}" "${myGeneratedSourceDir}")
	MakeGlobal(${myName}_AIWRAPPER_INCLUDE_DIRS)

	set(mySources
		"${mySourceDir}/AIEvent.cpp"
		"${mySourceDir}/AIFloat3.cpp"
		"${CMAKE_SOURCE_DIR}/rts/System/float3.cpp"
		"${mySourceDir}/AIColor.cpp"
		"${mySourceDir}/AIException.cpp"
		"${mySourceDir}/CallbackAIException.cpp"
		"${mySourceDir}/EventAIException.cpp"
		)

	set(myGeneratedCombineSources
		${myGeneratedSourceDir}/CombinedCallbackBridge.c)
	set(myGeneratedWrapperSources
		${myGeneratedSourceDir}/AbstractCamera.cpp
		${myGeneratedSourceDir}/AbstractCheats.cpp
		${myGeneratedSourceDir}/AbstractCommand.cpp
		${myGeneratedSourceDir}/AbstractCommandDescription.cpp
		${myGeneratedSourceDir}/AbstractDamage.cpp
		${myGeneratedSourceDir}/AbstractDataDirs.cpp
		${myGeneratedSourceDir}/AbstractDrawer.cpp
		${myGeneratedSourceDir}/AbstractEconomy.cpp
		${myGeneratedSourceDir}/AbstractEngine.cpp
		${myGeneratedSourceDir}/AbstractFeature.cpp
		${myGeneratedSourceDir}/AbstractFeatureDef.cpp
		${myGeneratedSourceDir}/AbstractFigure.cpp
		${myGeneratedSourceDir}/AbstractFile.cpp
		${myGeneratedSourceDir}/AbstractFlankingBonus.cpp
		${myGeneratedSourceDir}/AbstractGame.cpp
		${myGeneratedSourceDir}/AbstractGroup.cpp
		${myGeneratedSourceDir}/AbstractGui.cpp
		${myGeneratedSourceDir}/AbstractInfo.cpp
		${myGeneratedSourceDir}/AbstractLine.cpp
		${myGeneratedSourceDir}/AbstractLog.cpp
		${myGeneratedSourceDir}/AbstractLua.cpp
		${myGeneratedSourceDir}/AbstractMap.cpp
		${myGeneratedSourceDir}/AbstractMod.cpp
		${myGeneratedSourceDir}/AbstractMoveData.cpp
		${myGeneratedSourceDir}/AbstractOOAICallback.cpp
		${myGeneratedSourceDir}/AbstractOptionValues.cpp
		${myGeneratedSourceDir}/AbstractOrderPreview.cpp
		#${myGeneratedSourceDir}/AbstractPath.cpp
		${myGeneratedSourceDir}/AbstractPathDrawer.cpp
		${myGeneratedSourceDir}/AbstractPathing.cpp
		${myGeneratedSourceDir}/AbstractPoint.cpp
		${myGeneratedSourceDir}/AbstractResource.cpp
		${myGeneratedSourceDir}/AbstractRoots.cpp
		${myGeneratedSourceDir}/AbstractShield.cpp
		${myGeneratedSourceDir}/AbstractSkirmishAI.cpp
		${myGeneratedSourceDir}/AbstractSkirmishAIs.cpp
		${myGeneratedSourceDir}/AbstractTeam.cpp
		${myGeneratedSourceDir}/AbstractTeams.cpp
		${myGeneratedSourceDir}/AbstractUnit.cpp
		${myGeneratedSourceDir}/AbstractUnitDef.cpp
		${myGeneratedSourceDir}/AbstractVersion.cpp
		${myGeneratedSourceDir}/AbstractWeaponDef.cpp
		${myGeneratedSourceDir}/AbstractWeaponMount.cpp
		${myGeneratedSourceDir}/AbstractWeapon.cpp
		${myGeneratedSourceDir}/StubCamera.cpp
		${myGeneratedSourceDir}/StubCheats.cpp
		${myGeneratedSourceDir}/StubCommand.cpp
		${myGeneratedSourceDir}/StubCommandDescription.cpp
		${myGeneratedSourceDir}/StubDamage.cpp
		${myGeneratedSourceDir}/StubDataDirs.cpp
		${myGeneratedSourceDir}/StubDrawer.cpp
		${myGeneratedSourceDir}/StubEconomy.cpp
		${myGeneratedSourceDir}/StubEngine.cpp
		${myGeneratedSourceDir}/StubFeature.cpp
		${myGeneratedSourceDir}/StubFeatureDef.cpp
		${myGeneratedSourceDir}/StubFigure.cpp
		${myGeneratedSourceDir}/StubFile.cpp
		${myGeneratedSourceDir}/StubFlankingBonus.cpp
		${myGeneratedSourceDir}/StubGame.cpp
		${myGeneratedSourceDir}/StubGroup.cpp
		${myGeneratedSourceDir}/StubGui.cpp
		${myGeneratedSourceDir}/StubInfo.cpp
		${myGeneratedSourceDir}/StubLine.cpp
		${myGeneratedSourceDir}/StubLog.cpp
		${myGeneratedSourceDir}/StubLua.cpp
		${myGeneratedSourceDir}/StubMap.cpp
		${myGeneratedSourceDir}/StubMod.cpp
		${myGeneratedSourceDir}/StubMoveData.cpp
		${myGeneratedSourceDir}/StubOOAICallback.cpp
		${myGeneratedSourceDir}/StubOptionValues.cpp
		${myGeneratedSourceDir}/StubOrderPreview.cpp
		#${myGeneratedSourceDir}/StubPath.cpp
		${myGeneratedSourceDir}/StubPathDrawer.cpp
		${myGeneratedSourceDir}/StubPathing.cpp
		${myGeneratedSourceDir}/StubPoint.cpp
		${myGeneratedSourceDir}/StubResource.cpp
		${myGeneratedSourceDir}/StubRoots.cpp
		${myGeneratedSourceDir}/StubShield.cpp
		${myGeneratedSourceDir}/StubSkirmishAI.cpp
		${myGeneratedSourceDir}/StubSkirmishAIs.cpp
		${myGeneratedSourceDir}/StubTeam.cpp
		${myGeneratedSourceDir}/StubTeams.cpp
		${myGeneratedSourceDir}/StubUnit.cpp
		${myGeneratedSourceDir}/StubUnitDef.cpp
		${myGeneratedSourceDir}/StubVersion.cpp
		${myGeneratedSourceDir}/StubWeaponDef.cpp
		${myGeneratedSourceDir}/StubWeaponMount.cpp
		${myGeneratedSourceDir}/StubWeapon.cpp
		${myGeneratedSourceDir}/WrappCamera.cpp
		${myGeneratedSourceDir}/WrappCheats.cpp
		${myGeneratedSourceDir}/WrappCurrentCommand.cpp
		${myGeneratedSourceDir}/WrappDamage.cpp
		${myGeneratedSourceDir}/WrappDataDirs.cpp
		${myGeneratedSourceDir}/WrappDebug.cpp
		${myGeneratedSourceDir}/WrappDrawer.cpp
		${myGeneratedSourceDir}/WrappEconomy.cpp
		${myGeneratedSourceDir}/WrappEngine.cpp
		${myGeneratedSourceDir}/WrappFeature.cpp
		${myGeneratedSourceDir}/WrappFeatureDef.cpp
		${myGeneratedSourceDir}/WrappFigure.cpp
		${myGeneratedSourceDir}/WrappFile.cpp
		${myGeneratedSourceDir}/WrappFlankingBonus.cpp
		${myGeneratedSourceDir}/WrappGame.cpp
		${myGeneratedSourceDir}/WrappGraphDrawer.cpp
		${myGeneratedSourceDir}/WrappGraphLine.cpp
		${myGeneratedSourceDir}/WrappGroup.cpp
		${myGeneratedSourceDir}/WrappGroupSupportedCommand.cpp
		${myGeneratedSourceDir}/WrappGui.cpp
		${myGeneratedSourceDir}/WrappInfo.cpp
		${myGeneratedSourceDir}/WrappLine.cpp
		${myGeneratedSourceDir}/WrappLog.cpp
		${myGeneratedSourceDir}/WrappLua.cpp
		${myGeneratedSourceDir}/WrappMap.cpp
		${myGeneratedSourceDir}/WrappMod.cpp
		${myGeneratedSourceDir}/WrappMoveData.cpp
		${myGeneratedSourceDir}/WrappOOAICallback.cpp
		${myGeneratedSourceDir}/WrappOptionValues.cpp
		${myGeneratedSourceDir}/WrappOrderPreview.cpp
		#${myGeneratedSourceDir}/WrappPath.cpp
		${myGeneratedSourceDir}/WrappPathDrawer.cpp
		${myGeneratedSourceDir}/WrappPathing.cpp
		${myGeneratedSourceDir}/WrappPoint.cpp
		${myGeneratedSourceDir}/WrappResource.cpp
		${myGeneratedSourceDir}/WrappRoots.cpp
		${myGeneratedSourceDir}/WrappShield.cpp
		${myGeneratedSourceDir}/WrappSkirmishAI.cpp
		${myGeneratedSourceDir}/WrappSkirmishAIs.cpp
		${myGeneratedSourceDir}/WrappTeam.cpp
		${myGeneratedSourceDir}/WrappTeams.cpp
		${myGeneratedSourceDir}/WrappUnit.cpp
		${myGeneratedSourceDir}/WrappUnitDef.cpp
		${myGeneratedSourceDir}/WrappUnitSupportedCommand.cpp
		${myGeneratedSourceDir}/WrappVersion.cpp
		${myGeneratedSourceDir}/WrappWeaponDef.cpp
		${myGeneratedSourceDir}/WrappWeaponMount.cpp
		${myGeneratedSourceDir}/WrappWeapon.cpp)
	set(myGeneratedSources
		${myGeneratedCombineSources}
		${myGeneratedWrapperSources})
	set_source_files_properties(${myGeneratedSources} PROPERTIES GENERATED TRUE)


	set(commonAwkScriptArgs
		"-v" "SPRING_SOURCE_DIR=${springSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myGeneratedSourceDir}"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		)

	# 1. Combine Callback & Commands (C->C)
	add_custom_command(
		DEPENDS
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/combine_wrappCallback.awk"
			"${myAwkScriptsDir}/combine_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		OUTPUT
			"${myGeneratedSourceDir}/CombinedCallbackBridge.h"
			"${myGeneratedSourceDir}/CombinedCallbackBridge.c"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory" "${myGeneratedSourceDir}"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/combine_wrappCallback.awk"
			"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/combine_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating combined C callback wrapper sources ..." VERBATIM)

	# this will regenerate the sources whenever the DEPENDS changes
	add_custom_command(
		DEPENDS
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${commonAwkScriptsDir}/commonOOCallback.awk"
			"${myAwkScriptsDir}/wrappCallback.awk"
			"${myGeneratedSourceDir}/CombinedCallbackBridge.h"
			"${myGeneratedSourceDir}/CombinedCallbackBridge.c"
		OUTPUT
			${myGeneratedWrapperSources}
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${commonAwkScriptsDir}/commonOOCallback.awk"
			"-f" "${myAwkScriptsDir}/wrappCallback.awk"
			"${myGeneratedSourceDir}/CombinedCallbackBridge.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating C++ Callback wrapper sources" VERBATIM)

	add_custom_target(${myGenTarget} DEPENDS ${myGeneratedSources})
	add_dependencies(generateSources ${myGenTarget})

	# build the static lib
	include_directories(${mySourceDir} ${myGeneratedSourceDir})
	set(myCompleteSources ${mySources} ${myGeneratedSources})
	list(APPEND myCompleteSources ${ai_common_SRC})
	add_library(${myTarget} STATIC ${myCompleteSources})
	Add_Dependencies(${myTarget} generateVersionFiles)
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "${myName}")
endif (BUILD_${myName}_AIWRAPPER)

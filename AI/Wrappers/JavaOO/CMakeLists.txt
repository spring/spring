### Java OO AI Wrapper
#
# Global variables set in this file:
# * BUILD_JavaOO_AIWRAPPER
#

#enable_language(Java)

set(myName               "JavaOO")


# Check if the user wants to compile the wrapper
if    ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")
	set(AIWRAPPERS_JAVA TRUE)
else  ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")
	set(AIWRAPPERS_JAVA FALSE)
endif ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")


# Check dependencies of the wrapper are met
if    (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIWRAPPER TRUE)
else  (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIWRAPPER FALSE)
	message("warning: Java OO AI Wrapper will not be built!")
endif (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	set(myParentTarget     "${Java_AIINTERFACE_TARGET}")
	set(mySourceDirRel     "src/main/java")
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIWrapperMessage(STATUS "Found AI Wrapper: ${myName}")

	#SetGlobal(${myName}_AIWRAPPER_TARGET      ${myTarget})


	# Build library
	set(myPkgFirstPart               "com")
	set(myParentPkg                  "${myPkgFirstPart}/springrts/ai")
	set(myPkg                        "${myParentPkg}/oo")
	set(myBinDir                     "${myDir}/bin")
	set(commonAwkScriptsDir          "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myBuildDir                   "${CMAKE_CURRENT_BINARY_DIR}")
	set(myGeneratedSourceDir         "${myBuildDir}/src-generated/main/java")
	set(myJavaBuildDir               "${myBuildDir}/classes")
	set(myJarFile                    "${myName}-AIWrapper")
	set(myBinJarFile                 "${myJarFile}.jar")
	set(mySrcJarFile                 "${myJarFile}-src.jar")
	set(myJLibDir                    "${myDir}/jlib")
	set(myJLibs                      "${myJLibDir}/vecmath.jar")
	set(jAiIntJavaSourceDir          "${JAVA_SRC_DIR_Java_AIINTERFACE}")
	set(jAiIntJavaGeneratedSourceDir "${JAVA_GEN_SRC_DIR_Java_AIINTERFACE}")
	set(myClassPath                  "${myJLibs}${PATH_DELIM_H}${CLASSPATH_Java_AIINTERFACE}")

	SetGlobal(${myName}_AIWRAPPER_JAR_BIN       "${myBuildDir}/${myBinJarFile}")
	SetGlobal(${myName}_AIWRAPPER_JAR_SRC       "${myBuildDir}/${mySrcJarFile}")
	SetGlobal(${myName}_AIWRAPPER_JAR_CLASSPATH "${myJLibs}${PATH_DELIM_H}${${myName}_AIWRAPPER_JAR_BIN}")
	SetGlobal(${myName}_AIWRAPPER_TARGET        "${myTarget}")
	SetGlobal(SOURCE_ROOT_${myName}_AIWRAPPER   "${myDir}")
	SetGlobal(BUILD_ROOT_${myName}_AIWRAPPER    "${CMAKE_CURRENT_BINARY_DIR}")

	# Locate the manifest file
	FindManifestFile("${myDir}" myManifestFile)
	if    (myManifestFile)
		set(myBinJarArgs "cmf" "${myManifestFile}")
	else  (myManifestFile)
		set(myBinJarArgs "cf")
	endif (myManifestFile)


	# Generate sources
	# ----------------

	# We have to create these directories, cause otherwise the AWK scripts
	# for generating the source code will not work
	file(REMOVE_RECURSE "${myGeneratedSourceDir}")
	file(MAKE_DIRECTORY "${myGeneratedSourceDir}/${myPkg}/clb")
	file(MAKE_DIRECTORY "${myGeneratedSourceDir}/${myPkg}/evt")
	# needed by javac
	file(MAKE_DIRECTORY "${myJavaBuildDir}")

	set(commonAwkScriptArgs
		"-v" "INTERFACE_SOURCE_DIR=${jAiIntJavaSourceDir}"
		"-v" "INTERFACE_GENERATED_SOURCE_DIR=${jAiIntJavaGeneratedSourceDir}"
		"-v" "JAVA_GENERATED_SOURCE_DIR=${myGeneratedSourceDir}"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		)

	set(mySources
		"${mySourceDir}/${myPkg}/AIEvent.java"
		"${mySourceDir}/${myPkg}/AIFloat3.java"
		"${mySourceDir}/${myPkg}/AIException.java"
		"${mySourceDir}/${myPkg}/CallbackAIException.java"
		"${mySourceDir}/${myPkg}/EventAIException.java"
		)

	set(myGeneratedCallbackSources
		"${myGeneratedSourceDir}/${myPkg}/clb/Camera.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Cheats.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/CollisionVolume.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/CommandDescription.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Command.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Damage.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/DataDirs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Drawer.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Economy.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Engine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/FeatureDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Feature.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Figure.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/FlankingBonus.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Game.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Group.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Gui.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Info.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Line.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Log.java"
#		"${myGeneratedSourceDir}/${myPkg}/clb/Lua.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Map.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Mod.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/ModParam.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/MoveData.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/OOAICallback.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/OptionValues.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/OrderPreview.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Pathing.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Path.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Point.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Resource.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Roots.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Shield.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/SkirmishAI.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/SkirmishAIs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Teams.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/UnitDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Unit.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/Version.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WeaponDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WeaponMount.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractCamera.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractCheats.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractCollisionVolume.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractCommandDescription.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractCommand.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractDamage.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractDataDirs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractDrawer.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractEconomy.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractEngine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractFeatureDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractFeature.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractFigure.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractFlankingBonus.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractGame.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractGroup.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractGui.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractInfo.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractLine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractLog.java"
#		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractLua.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractMap.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractMod.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractModParam.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractMoveData.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractOOAICallback.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractOptionValues.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractOrderPreview.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractPathing.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractPath.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractPoint.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractResource.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractRoots.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractShield.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractSkirmishAI.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractSkirmishAIs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractTeams.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractUnitDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractUnit.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractVersion.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractWeaponDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/AbstractWeaponMount.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubCamera.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubCheats.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubCollisionVolume.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubCommandDescription.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubCommand.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubDamage.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubDataDirs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubDrawer.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubEconomy.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubEngine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubFeatureDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubFeature.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubFigure.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubFlankingBonus.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubGame.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubGroup.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubGui.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubInfo.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubLine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubLog.java"
#		"${myGeneratedSourceDir}/${myPkg}/clb/StubLua.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubMap.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubMod.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubModParam.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubMoveData.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubOOAICallback.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubOptionValues.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubOrderPreview.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubPathing.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubPath.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubPoint.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubResource.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubRoots.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubShield.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubSkirmishAI.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubSkirmishAIs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubTeams.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubUnitDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubUnit.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubVersion.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubWeaponDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/StubWeaponMount.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappCamera.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappCheats.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappCurrentCommand.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappDamage.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappDataDirs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappDrawer.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappEconomy.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappEngine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappFeatureDefCollisionVolume.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappFeatureDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappFeature.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappFigure.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappFlankingBonus.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappGame.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappGroup.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappGroupSupportedCommand.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappGui.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappInfo.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappLine.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappLog.java"
#		"${myGeneratedSourceDir}/${myPkg}/clb/WrappLua.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappMap.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappMod.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappModParam.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappMoveData.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappOOAICallback.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappOptionValues.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappOrderPreview.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappPathing.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappPath.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappPoint.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappResource.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappRoots.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappShield.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappSkirmishAI.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappSkirmishAIs.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappTeams.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnitDefCollisionVolume.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnitDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnit.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnitSupportedCommand.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappVersion.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappWeaponDef.java"
		"${myGeneratedSourceDir}/${myPkg}/clb/WrappWeaponMount.java"
		)

	set(myGeneratedEventSources
		"${myGeneratedSourceDir}/${myPkg}/AbstractOOAI.java"
		"${myGeneratedSourceDir}/${myPkg}/IOOAI.java"
		"${myGeneratedSourceDir}/${myPkg}/OOAI.java"
		"${myGeneratedSourceDir}/${myPkg}/IOOEventAI.java"
		"${myGeneratedSourceDir}/${myPkg}/OOEventAI.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitLifeStateAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitTeamChangeAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/LoadSaveAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/CommandFinishedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyDamagedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyDestroyedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyEnterLOSAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyEnterRadarAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyLeaveLOSAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyLeaveRadarAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/InitAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/LoadAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/MessageAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/PlayerCommandAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/ReleaseAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/SaveAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/SeismicPingAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitCapturedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitCreatedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitDamagedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitDestroyedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitFinishedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitGivenAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitIdleAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UnitMoveFailedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/UpdateAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/WeaponFiredAIEvent.java"
		)

	set(myGeneratedSources
		${myGeneratedCallbackSources}
		${myGeneratedEventSources}
		)
	set_source_files_properties(${myGeneratedSources} PROPERTIES GENERATED TRUE)



	add_custom_target(${myTarget} ALL)
	add_dependencies(${myTarget} ${myParentTarget})

	# generate sources: callback
	add_custom_command(
		#TARGET ${myParentTarget}
		TARGET ${myTarget}
		#OUTPUT ${myGeneratedCallbackSources}
		DEPENDS "${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
		#	"${myParentTarget}"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${commonAwkScriptsDir}/commonOOCallback.awk"
			"-f" "${myBinDir}/wrappCallback.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
		WORKING_DIRECTORY "${myBinDir}"
		COMMENT "  ${myTarget}: Generating callback sources" VERBATIM
		)

	# generate sources: events
	add_custom_command(
		#TARGET ${myParentTarget}
		TARGET ${myTarget}
		#OUTPUT ${myGeneratedEventSources}
		DEPENDS "${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AI.java"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myBinDir}/wrappEvents.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AI.java"
		WORKING_DIRECTORY "${myBinDir}"
		COMMENT "  ${myTarget}: Generating event sources" VERBATIM
		)


	# Build the jars
	# --------------

	# compile
	add_custom_command(
		#TARGET ${myParentTarget}
		TARGET ${myTarget}
		#OUTPUT ${myClasses}
		DEPENDS
			${mySources}
			${myGeneratedSources}
		COMMAND "${JAVA_COMPILE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-cp" "${myClassPath}"
			"-d" "${myJavaBuildDir}"
			${mySources}
			${myGeneratedSources}
		WORKING_DIRECTORY "${myBinDir}"
		COMMENT "  ${myTarget}: Compiling sources" VERBATIM
		)

	# pack library
	add_custom_command(
		#TARGET ${myParentTarget}
		TARGET ${myTarget}
		#OUTPUT "${myBuildDir}/${myBinJarFile}"
		#DEPENDS "${myClasses}"
		COMMAND "${JAVA_ARCHIVE}" ARGS
			${myBinJarArgs} "${${myName}_AIWRAPPER_JAR_BIN}"
			"-C" "${myJavaBuildDir}" "${myPkgFirstPart}"
		WORKING_DIRECTORY "${myBuildDir}"
		COMMENT "  ${myTarget}: Creating library ${myBinJarFile} ..." VERBATIM)

	# pack sources
	add_custom_command(
		#TARGET ${myParentTarget}
		TARGET ${myTarget}
		#OUTPUT "${myBuildDir}/${mySrcJarFile}"
		DEPENDS
			${myJavaSources}
			${myJavaGeneratedSources}
		COMMAND "${JAVA_ARCHIVE}" ARGS
			"cf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${mySourceDir}" "${myPkgFirstPart}"
		COMMAND "${JAVA_ARCHIVE}" ARGS
			"uf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${myGeneratedSourceDir}" "${myPkgFirstPart}"
		WORKING_DIRECTORY "${myBuildDir}"
		COMMENT "  ${myTarget}: Creating sources archive ${mySrcJarFile} ..." VERBATIM)

endif (BUILD_${myName}_AIWRAPPER)

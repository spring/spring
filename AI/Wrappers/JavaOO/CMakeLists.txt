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
if    (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}" AND CUSTOM_TARGET_SOURCES_SUPPORT)
	SetGlobal(BUILD_${myName}_AIWRAPPER TRUE)
else  (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}" AND CUSTOM_TARGET_SOURCES_SUPPORT)
	SetGlobal(BUILD_${myName}_AIWRAPPER FALSE)
	message("warning: Java OO AI Wrapper will not be built!")
endif (AIWRAPPERS_JAVA AND BUILD_Java_AIINTERFACE AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}" AND CUSTOM_TARGET_SOURCES_SUPPORT)


# Build
if    (BUILD_${myName}_AIWRAPPER)
	set(myDir        "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName       "${dirName}")
	set(myTarget     "${myName}-AIWrapper")
	set(mySourceDirRel     "src/main/java")
	MakeAbsolute(mySourceDir "${myDir}" "${mySourceDirRel}")

	AIWrapperMessage(STATUS "Found AI Wrapper: ${myName}")


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
	FindJavaLib(vecmath_jar "vecmath" "${myJLibDir}")
	set(myJLibs                      "${vecmath_jar}")
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

	set(myGenClbClasses
		"Camera"
		"Cheats"
		"CollisionVolume"
		"CommandDescription"
		"Command"
		"Damage"
		"DataDirs"
		"Debug"
		"Drawer"
		"Economy"
		"Engine"
		"FeatureDef"
		"Feature"
		"Figure"
		"FlankingBonus"
		"Game"
		"GraphDrawer"
		"GraphLine"
		"Group"
		"Gui"
		"Info"
		"Line"
		"Log"
#		"Lua"
		"Map"
		"Mod"
		"ModParam"
		"MoveData"
		"OOAICallback"
		"OptionValues"
		"OrderPreview"
		"Pathing"
		"PathDrawer"
		"Point"
		"Resource"
		"Roots"
		"Shield"
		"SkirmishAI"
		"SkirmishAIs"
		"Teams"
		"OverlayTexture"
		"UnitDef"
		"Unit"
		"Version"
		"WeaponDef"
		"WeaponMount"
		)
	set(myGeneratedCallbackSources )
	foreach    (className ${myGenClbClasses})
		list(APPEND myGeneratedCallbackSources
			"${myGeneratedSourceDir}/${myPkg}/clb/${className}.java"
			"${myGeneratedSourceDir}/${myPkg}/clb/Abstract${className}.java"
			"${myGeneratedSourceDir}/${myPkg}/clb/Stub${className}.java"
			)
		if     ("${className}" STREQUAL "CollisionVolume")
			list(APPEND myGeneratedCallbackSources
				"${myGeneratedSourceDir}/${myPkg}/clb/WrappFeatureDef${className}.java"
				"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnitDef${className}.java"
				)
		elseif ("${className}" STREQUAL "CommandDescription")
			list(APPEND myGeneratedCallbackSources
				"${myGeneratedSourceDir}/${myPkg}/clb/WrappGroupSupportedCommand.java"
				"${myGeneratedSourceDir}/${myPkg}/clb/WrappUnitSupportedCommand.java"
				)
		elseif ("${className}" STREQUAL "Command")
			list(APPEND myGeneratedCallbackSources
				"${myGeneratedSourceDir}/${myPkg}/clb/WrappCurrentCommand.java"
				)
		else   ()
			list(APPEND myGeneratedCallbackSources
				"${myGeneratedSourceDir}/${myPkg}/clb/Wrapp${className}.java"
				)
		endif  ()
	endforeach (className)

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


	# Assemble project generated targets (and their libraries) we depend on
	set(myDependTargets  "${Java_AIINTERFACE_TARGET}")
	set(myDependLibFiles "${Java_AIINTERFACE_JAR_BIN}")
	set_source_files_properties(${myDependLibFiles} PROPERTIES GENERATED TRUE)

	add_custom_target(${myTarget} ALL
		SOURCES
			"${myBuildDir}/${myBinJarFile}"
			"${myBuildDir}/${mySrcJarFile}")
	add_dependencies(${myTarget} ${myDependTargets})


	# generate sources: callback
	add_custom_command(
		OUTPUT
			${myGeneratedCallbackSources}
		COMMAND "${CMAKE_COMMAND}"
			"-E" "remove_directory" "${myGeneratedSourceDir}/${myPkg}/clb"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory"   "${myGeneratedSourceDir}/${myPkg}/clb"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${commonAwkScriptsDir}/commonOOCallback.awk"
			"-f" "${myBinDir}/wrappCallback.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
		DEPENDS
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${commonAwkScriptsDir}/commonOOCallback.awk"
			"${myBinDir}/wrappCallback.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
		WORKING_DIRECTORY
			"${myBinDir}"
		COMMENT
			"  ${myTarget}: Generating callback sources" VERBATIM
		)

	# generate sources: events
	add_custom_command(
		OUTPUT
			${myGeneratedEventSources}
		COMMAND "${CMAKE_COMMAND}"
			"-E" "remove_directory" "${myGeneratedSourceDir}/${myPkg}/evt"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory"   "${myGeneratedSourceDir}/${myPkg}/evt"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myBinDir}/wrappEvents.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AI.java"
		DEPENDS
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myBinDir}/wrappEvents.awk"
			"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AI.java"
		WORKING_DIRECTORY
			"${myBinDir}"
		COMMENT
			"  ${myTarget}: Generating event sources" VERBATIM
		)


	# Build the jars
	# --------------

	# Stub file for dependency tracking
	set(myClassesStubFile "${CMAKE_CURRENT_BINARY_DIR}/myClasses.stub")
	set_source_files_properties(${myClassesStubFile} PROPERTIES GENERATED TRUE)

	# compile
	add_custom_command(
		OUTPUT
			${myClassesStubFile}
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory" "${myJavaBuildDir}"
		COMMAND "${JAVA_COMPILE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-cp" "${myClassPath}"
			"-d" "${myJavaBuildDir}"
			${mySources}
			${myGeneratedSources}
		COMMAND "${CMAKE_COMMAND}"
			"-E" "touch" "${myClassesStubFile}"
		DEPENDS
			${myDependLibFiles}
			${mySources}
			${myGeneratedSources}
		WORKING_DIRECTORY
			"${myBinDir}"
		COMMENT
			"  ${myTarget}: Compiling sources" VERBATIM
		)

	# pack library
	add_custom_command(
		OUTPUT
			"${myBuildDir}/${myBinJarFile}"
		COMMAND "${JAVA_ARCHIVE}" ARGS
			${myBinJarArgs} "${${myName}_AIWRAPPER_JAR_BIN}"
			"-C" "${myJavaBuildDir}" "${myPkgFirstPart}"
		DEPENDS
			"${myClassesStubFile}"
		WORKING_DIRECTORY
			"${myBuildDir}"
		COMMENT
			"  ${myTarget}: Creating library ${myBinJarFile} ..." VERBATIM
		)

	# pack sources
	add_custom_command(
		OUTPUT
			"${myBuildDir}/${mySrcJarFile}"
		COMMAND "${JAVA_ARCHIVE}" ARGS
			"cf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${mySourceDir}" "${myPkgFirstPart}"
		COMMAND "${JAVA_ARCHIVE}" ARGS
			"uf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${myGeneratedSourceDir}" "${myPkgFirstPart}"
		DEPENDS
			${myJavaSources}
			${myJavaGeneratedSources}
		WORKING_DIRECTORY
			"${myBuildDir}"
		COMMENT
			"  ${myTarget}: Creating sources archive ${mySrcJarFile} ..." VERBATIM
		)

endif (BUILD_${myName}_AIWRAPPER)

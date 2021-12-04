### Java OO AI Wrapper
#
# Global variables set in this file:
# * BUILD_JavaOO_AIWRAPPER
#

#enable_language(Java)

# includes rts/build/cmake/UtilJava.cmake
INCLUDE(UtilJava)


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
	set(myGenTarget  "${myTarget}-generateSources")
	set(myJavaTarget "${myTarget}-java")
	set(myPomTarget  "${myTarget}-pom")
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

	# remove all generated sources from build dir, if it exists
	# (required for build dirs of git:master from before 21. September 2010)
	file(REMOVE_RECURSE "${myGeneratedSourceDir}/${myPkg}")


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
		"Lua"
		"Map"
		"Mod"
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
		"Team"
		"Teams"
		"OverlayTexture"
		"UnitDef"
		"Unit"
		"Version"
		"WeaponDef"
		"WeaponMount"
		"Weapon"
		)
	set(myGeneratedCallbackSources )
	foreach    (className ${myGenClbClasses})
		list(APPEND myGeneratedCallbackSources
			"${myGeneratedSourceDir}/${myPkg}/clb/${className}.java"
			"${myGeneratedSourceDir}/${myPkg}/clb/Abstract${className}.java"
			"${myGeneratedSourceDir}/${myPkg}/clb/Stub${className}.java"
			)
		if ("${className}" STREQUAL "CommandDescription")
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
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyCreatedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyDamagedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyDestroyedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyFinishedAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyEnterLOSAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyEnterRadarAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyLeaveLOSAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/EnemyLeaveRadarAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/InitAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/LoadAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/MessageAIEvent.java"
		"${myGeneratedSourceDir}/${myPkg}/evt/LuaMessageAIEvent.java"
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

	set(jAiIntJavaGeneratedSources
		"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AI.java"
		"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
		)
	set_source_files_properties(${jAiIntJavaGeneratedSources} PROPERTIES GENERATED TRUE)


	# Assemble project generated targets (and their libraries) we depend on
	set(myDependTargets  "${Java_AIINTERFACE_TARGET}")
	set(myDependLibFiles "${Java_AIINTERFACE_JAR_BIN}")
	set_source_files_properties(${myDependLibFiles} PROPERTIES GENERATED TRUE)

	add_custom_target(${myJavaTarget}
		DEPENDS
			"${myBuildDir}/${myBinJarFile}"
			"${myBuildDir}/${mySrcJarFile}")
	add_dependencies(${myJavaTarget} ${myDependTargets})


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
			# Handled through a target level dependency, see below
			#"${jAiIntJavaGeneratedSourceDir}/${myParentPkg}/AICallback.java"
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

	add_custom_target(${myGenTarget} DEPENDS ${myGeneratedSources})
	add_dependencies(${myGenTarget} ${Java_AIINTERFACE_TARGET_GENERATE_SOURCES}) # for AICallback.java
	add_dependencies(generateSources ${myGenTarget})


	# Build the jars
	# --------------

	# Write list of source files to an arg-file
	set(mySrcArgFile "${CMAKE_CURRENT_BINARY_DIR}/sourceFiles.txt")
	if    (EXISTS "${mySrcArgFile}")
		file(REMOVE "${mySrcArgFile}")
	endif (EXISTS "${mySrcArgFile}")
	foreach    (srcFile ${mySources} ${myGeneratedSources})
		file(APPEND "${mySrcArgFile}" "\"${srcFile}\"\n")
	endforeach (srcFile)

	# compile and pack library
	add_custom_command(
		OUTPUT
			"${myBuildDir}/${myBinJarFile}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "remove_directory" "${myJavaBuildDir}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory" "${myJavaBuildDir}"
		COMMAND "${Java_JAVAC_EXECUTABLE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-Xlint:deprecation"
			"-cp" "${myClassPath}"
			"-d" "${myJavaBuildDir}"
			"@${mySrcArgFile}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			${myBinJarArgs} "${myBuildDir}/${myBinJarFile}"
			"-C" "${myJavaBuildDir}" "${myPkgFirstPart}"
		DEPENDS
			${myDependLibFiles}
			${mySources}
			${myGeneratedSources}
		WORKING_DIRECTORY
			"${myBinDir}"
		COMMENT
			"  ${myTarget}: Compiling sources and packing library ${myBinJarFile}" VERBATIM
		)

	# pack sources
	add_custom_command(
		OUTPUT
			"${myBuildDir}/${mySrcJarFile}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"cf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${mySourceDir}"          "${myPkgFirstPart}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"uf" "${${myName}_AIWRAPPER_JAR_SRC}"
			"-C" "${myGeneratedSourceDir}" "${myPkgFirstPart}"
		DEPENDS
			${myJavaSources}
			${myJavaGeneratedSources}
		WORKING_DIRECTORY
			"${myBuildDir}"
		COMMENT
			"  ${myTarget}: Creating sources archive ${mySrcJarFile}" VERBATIM
		)

	# This sets the version in pom.xml
	# as we have no separate version for the wrapper,
	# we use the one from the interrface
	set(myMavenProperties "-Dmy.version=${Java_AIINTERFACE_VERS}")
	add_custom_command(
		OUTPUT
			"${myBuildDir}/pom-generated.xml"
		COMMAND "${CMAKE_COMMAND}"
			"-Dfile.in=${myDir}/pom.xml"
			"-Dfile.out=${myBuildDir}/pom-generated.xml"
			${myMavenProperties}
			"-P" "${CMAKE_MODULES_SPRING}/ConfigureFile.cmake"
		DEPENDS
			"${myDir}/pom.xml"
		WORKING_DIRECTORY
			"${myDir}"
		COMMENT
			"  ${myTarget}: Configure pom.xml" VERBATIM
		)
	set_source_files_properties("${myBuildDir}/pom-generated.xml" PROPERTIES GENERATED TRUE)

	add_custom_target(${myPomTarget}
		DEPENDS
			"${myBuildDir}/pom-generated.xml")

	add_custom_target(${myTarget} ALL)

	add_dependencies(${myTarget} ${myJavaTarget})

endif (BUILD_${myName}_AIWRAPPER)

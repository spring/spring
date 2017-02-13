### Java AI Interface
#
# Global variables set in this file:
# * BUILD_Java_AIINTERFACE
# * Java_AIINTERFACE_VERS
# * Java_AIINTERFACE_TARGET
#
# Functions and macros defined in this file:
# * ConfigureJavaSkirmishAI
#

#enable_language(Java)


################################################################################
### BEGINN: MACROS_AND_FUNCTIONS
# Define macros and functions to be used in this file and by Java Skirmish AIs

# includes rts/build/cmake/UtilJava.cmake
INCLUDE(UtilJava)

# Java Skirmish AI configuration macro.
# This will be called from Java AIs at AI/Skirmish/*/CMakeLists.txt.
macro    (ConfigureJavaSkirmishAI wrapperNames)
	# Assemble meta data
	set(myDir            "${CMAKE_CURRENT_SOURCE_DIR}")
	GetLastPathPart(dirName ${myDir})
	set(myName          "${dirName}")
	set(myJarFile       "SkirmishAI")
	set(myBinJarFile    "${myJarFile}.jar")
	set(mySrcJarFile    "${myJarFile}-src.jar")
	set(myJLibDir       "${myDir}/data/jlib")
	GetVersionPlusDepFile(myVersion myVersionDepFile)
	set(myTarget        "${myName}")
	set(myInstLibsDir   "${SKIRMISH_AI_LIBS}/${myName}/${myVersion}")
	set(myInstDataDir   "${SKIRMISH_AI_DATA}/${myName}/${myVersion}")
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish/${myName}
	set(myBuildDir      "${CMAKE_CURRENT_BINARY_DIR}")

	# Check if the user wants to compile the AI
	Set(BUILD_THIS_SKIRMISHAI FALSE)
	if    (BUILD_Java_AIINTERFACE AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
		set(BUILD_THIS_SKIRMISHAI TRUE)
	endif (BUILD_Java_AIINTERFACE AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	If    (NOT BUILD_THIS_SKIRMISHAI)
		Message("warning: ${myName} Skirmish AI will not be built!")
	EndIf (NOT BUILD_THIS_SKIRMISHAI)

	# Compile and install
	if    (BUILD_THIS_SKIRMISHAI)
		file(MAKE_DIRECTORY "${myBuildDir}/jlib")

		set(configString "default")
		SkirmishAIMessage(STATUS "Found Skirmish AI: ${myName} ${myVersion} (config: ${configString})")

		# Assemble project generated targets (and their libraries) we depend on
		set(myDependTargets  "${Java_AIINTERFACE_TARGET}")
		set(myDependLibFiles "${Java_AIINTERFACE_JAR_BIN}")
		foreach    (wrapperName ${wrapperNames})
			set(myDependTargets  ${myDependTargets}  "${${wrapperName}_AIWRAPPER_TARGET}")
			set(myDependLibFiles ${myDependLibFiles} "${${wrapperName}_AIWRAPPER_JAR_BIN}")
		endforeach (wrapperName)
		set_source_files_properties(${myDependLibFiles} PROPERTIES GENERATED TRUE)

		# find java source root
		if     (EXISTS "${myDir}/src/main/java")
			# default Maven source dir
			set(mySourceDir          "${myDir}/src/main/java")
		elseif (EXISTS "${myDir}/src")
			# simple java source dir path
			set(mySourceDir          "${myDir}/src")
		else   (EXISTS "${myDir}/src/main/java")
			message(SEND_ERROR "No sources dir found for Skirmish AI: ${myName}")
		endif  (EXISTS "${myDir}/src/main/java")

		# Create a list of all the AIs source files (for compiling)
		file(GLOB_RECURSE mySources RELATIVE "${mySourceDir}" FOLLOW_SYMLINKS "${mySourceDir}/*.java")
		# Create a list of all the AIs source files (for dependency tracking)
		file(GLOB_RECURSE mySourcesDep FOLLOW_SYMLINKS "${mySourceDir}/*.java")

		set_source_files_properties("${myBuildDir}/${myBinJarFile}" PROPERTIES GENERATED TRUE)
		set_source_files_properties("${myBuildDir}/${mySrcJarFile}" PROPERTIES GENERATED TRUE)


		# If main Java package is "my.ai.pkg", this has to be set to "my".
		GetFirstSubDirName(firstSrcSubDir ${mySourceDir})
		set(myJavaPkgFirstPart "${firstSrcSubDir}")

		# Assemble additional meta data
		set(myBinaryTarget "${myTarget}-BIN")
		set(mySourceTarget "${myTarget}-SRC")
		set(myJavaBuildDir "${myBuildDir}/classes")

		add_custom_target(${myBinaryTarget}
			DEPENDS "${myBuildDir}/${myBinJarFile}")
		add_dependencies(${myBinaryTarget} ${myDependTargets})

		add_custom_target(${mySourceTarget}
			DEPENDS "${myBuildDir}/${mySrcJarFile}")

		add_custom_target(${myTarget} ALL DEPENDS ${myVersionDepFile})
		add_dependencies(${myTarget} ${myBinaryTarget} ${mySourceTarget})

		# Create our full Java class-path
		CreateClasspath(myJavaLibs ${myJLibDir})
		ConcatClasspaths(myClassPath "${CLASSPATH_Java_AIINTERFACE}" "${myJavaLibs}")
		foreach    (wrapperName ${wrapperNames})
			ConcatClasspaths(myClassPath "${myClassPath}" "${${wrapperName}_AIWRAPPER_JAR_CLASSPATH}")
		endforeach (wrapperName)

		# Locate the manifest file
		FindManifestFile("${myDir}" myManifestFile)
		if    (myManifestFile)
			set(myBinJarArgs "cmf" "${myManifestFile}")
		else  (myManifestFile)
			set(myBinJarArgs "cf")
		endif (myManifestFile)

		# Write list of source files to an arg-file
		set(mySrcArgFile "${myBuildDir}/sourceFiles.txt")
		if    (EXISTS "${mySrcArgFile}")
			file(REMOVE "${mySrcArgFile}")
		endif (EXISTS "${mySrcArgFile}")
		set(mySrcArgFile "${myBuildDir}/sourceFiles.txt")
		foreach    (srcFile ${mySources})
			file(APPEND "${mySrcArgFile}" "\"${srcFile}\"\n")
		endforeach (srcFile)

		# Compile and pack the library
		add_custom_command(
			OUTPUT
				"${myBuildDir}/${myBinJarFile}"
			DEPENDS
				${myDependLibFiles}
				${mySourcesDep}
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
			COMMAND "${Java_JAR_EXECUTABLE}"
				${myBinJarArgs} "${myBuildDir}/${myBinJarFile}"
				"-C" "${myJavaBuildDir}" "${myJavaPkgFirstPart}"
			WORKING_DIRECTORY
				"${mySourceDir}"
			COMMENT
				"  ${myTarget}: Compiling sources and packing library ${myBinJarFile}" VERBATIM
			)

		# Pack the sources
		add_custom_command(
			OUTPUT
				"${myBuildDir}/${mySrcJarFile}"
			COMMAND "${Java_JAR_EXECUTABLE}"
				"cf" "${myBuildDir}/${mySrcJarFile}"
				"-C" "${mySourceDir}" "${myJavaPkgFirstPart}"
			DEPENDS
				${mySourcesDeps}
			WORKING_DIRECTORY
				"${mySourceDir}"
			COMMENT
				"  ${myTarget}: Creating sources archive ${mySrcJarFile}" VERBATIM
			)

		# Install the data files
		install(DIRECTORY "${myDir}/data/"                 DESTINATION ${myInstDataDir})
		# Install the library
		install(FILES    "${myBuildDir}/${myBinJarFile}"   DESTINATION ${myInstDataDir})
		# Install the sources archive (optional)
		install(FILES    "${myBuildDir}/${mySrcJarFile}"   DESTINATION ${myInstDataDir}/jlib OPTIONAL)

		# Install files generated/downloaded during buildtime
		install(DIRECTORY "${myBuildDir}/jlib/"            DESTINATION ${myInstDataDir}/jlib)
		if    (EXISTS "${myBuildDir}/resources")
			install(DIRECTORY "${myBuildDir}/resources/"   DESTINATION ${myInstDataDir}/resources)
		endif (EXISTS "${myBuildDir}/resources")
		if    (EXISTS "${myBuildDir}/config")
			install(DIRECTORY "${myBuildDir}/config/"      DESTINATION ${myInstDataDir}/config)
		endif (EXISTS "${myBuildDir}/config")
		if    (EXISTS "${myBuildDir}/script")
			install(DIRECTORY "${myBuildDir}/script/"      DESTINATION ${myInstDataDir}/script)
		endif (EXISTS "${myBuildDir}/script")

		# Install special script and config files
		if    (EXISTS "${myDir}/src/main/groovy")
			install(DIRECTORY "${myDir}/src/main/groovy/"  DESTINATION ${myInstDataDir}/script)
		endif (EXISTS "${myDir}/src/main/groovy")
		if    (EXISTS "${myDir}/src/main/ruby")
			install(DIRECTORY "${myDir}/src/main/ruby/"    DESTINATION ${myInstDataDir}/script)
		endif (EXISTS "${myDir}/src/main/ruby")
		if    (EXISTS "${myDir}/src/main/bsh")
			install(DIRECTORY "${myDir}/src/main/bsh/"     DESTINATION ${myInstDataDir}/script)
		endif (EXISTS "${myDir}/src/main/bsh")
		if    (EXISTS "${myDir}/src/main/config")
			install(DIRECTORY "${myDir}/src/main/config/"  DESTINATION ${myInstDataDir}/config)
		endif (EXISTS "${myDir}/src/main/config")

		# Install jars of wrappers
		foreach    (wrapperName ${wrapperNames})
			# Install the wrappers main jar
			install(FILES "${${wrapperName}_AIWRAPPER_JAR_BIN}" DESTINATION ${myInstDataDir}/jlib)
			set(wrapperJLibDir "${CMAKE_SOURCE_DIR}/AI/Wrappers/${wrapperName}/jlib")
			# Install the wrappers java libs, if there are any
			if    (EXISTS ${wrapperJLibDir})
				install(DIRECTORY "${wrapperJLibDir}/" DESTINATION ${myInstDataDir}/jlib)
			endif (EXISTS ${wrapperJLibDir})
		endforeach (wrapperName)
	endif (BUILD_THIS_SKIRMISHAI)
endmacro (ConfigureJavaSkirmishAI wrapperNames)


### END: MACROS_AND_FUNCTIONS
################################################################################


set(myName               "Java")
set(myDir                "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDirRel       "src/main")
set(myJavaSourceDirRel   "src/main/java")
set(myNativeSourceDirRel "src/main/native")
set(myPkgFirstPart       "com")
set(myPkg                "${myPkgFirstPart}/springrts/ai")


# Check if the user wants to compile the interface
if    ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")
	set(AI_TYPES_JAVA TRUE)
else  ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")
	set(AI_TYPES_JAVA FALSE)
endif ("${AI_TYPES}" STREQUAL "ALL" OR "${AI_TYPES}" STREQUAL "JAVA")

if    (AI_TYPES_JAVA AND myName MATCHES "${AI_EXCLUDE_REGEX}")
	set(AI_TYPES_JAVA FALSE)
endif (AI_TYPES_JAVA AND myName MATCHES "${AI_EXCLUDE_REGEX}")


# Look for dependencies, but only if the user wants to build the interface
if    (AI_TYPES_JAVA)
	if    (NOT JAVA_FOUND)
		FIND_PACKAGE(Java REQUIRED COMPONENTS Development)
	endif (NOT JAVA_FOUND)
	if    (MINGW)
		set (JNI_FOUND TRUE)
	else  (MINGW)
		# this hack is needed for FindJNI.cmake to use the JDK we want it to use,
		# as otherwise it might not find one at all (eg. in the case of OpenJDK)
		if    ( NOT ENV{JAVA_HOME} AND JAVA_HOME )
			SET(ENV{JAVA_HOME} "${JAVA_HOME}")
		endif ( NOT ENV{JAVA_HOME} AND JAVA_HOME )
		FIND_PACKAGE(JNI REQUIRED)
		if (JAVA_INCLUDE_PATH)
			set (JNI_FOUND TRUE)
			include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${JNI_INCLUDE_DIRS})
		else (JAVA_INCLUDE_PATH)
			set (JNI_FOUND FALSE)
			Message(WARNING "No Java includes found!")
		endif (JAVA_INCLUDE_PATH)
	endif (MINGW)
endif (AI_TYPES_JAVA)


# Check if dependencies of the interface are met
if    (AI_TYPES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIINTERFACE TRUE)
else  (AI_TYPES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")
	SetGlobal(BUILD_${myName}_AIINTERFACE FALSE)
	Message("warning: Java AI Interface will not be built!")
endif (AI_TYPES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin AND AWK_FOUND AND NOT myName MATCHES "${AI_EXCLUDE_REGEX}")


# Build
if    (BUILD_${myName}_AIINTERFACE)
	GetVersionPlusDepFile(myVersion myVersionDepFile)
	set(myTarget       "${myName}-AIInterface")
	set(myGenTarget    "${myTarget}-generateSources")
	set(myNativeTarget "${myTarget}-native")
	set(myJavaTarget   "${myTarget}-java")
	set(myInstLibsDir ${AI_INTERFACES_LIBS}/${myName}/${myVersion})
	set(myInstDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})
	set(myJavaSourceDirRel   "${mySourceDirRel}/java")
	set(myNativeSourceDirRel "${mySourceDirRel}/native")
	MakeAbsolute(mySourceDir       "${myDir}" "${mySourceDirRel}")
	MakeAbsolute(myNativeSourceDir "${myDir}" "${myNativeSourceDirRel}")
	MakeAbsolute(myJavaSourceDir   "${myDir}" "${myJavaSourceDirRel}")

	AIInterfaceMessage(STATUS "Found AI Interface: ${myTarget} ${myVersion}")

	SetGlobal(${myName}_AIINTERFACE_VERS   ${myVersion})
	SetGlobal(${myName}_AIINTERFACE_TARGET ${myTarget})
	SetGlobal(${myName}_AIINTERFACE_TARGET_GENERATE_SOURCES ${myGenTarget})


	# Init some vars
	# --------------
	set(myAwkScriptsDir            "${myDir}/bin")
	set(commonAwkScriptsDir        "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myBuildDir                 "${CMAKE_CURRENT_BINARY_DIR}")
	set(springSourceDir            "${PROJECT_SOURCE_DIR}")
	set(springAIInterfaceSourceDir "${springSourceDir}/rts/ExternalAI/Interface")
	set(myJavaBuildDir             "${myBuildDir}/classes")
	set(myJarFile                  "AIInterface")
	set(myBinJarFile               "${myJarFile}.jar")
	set(mySrcJarFile               "${myJarFile}-src.jar")
	set(myGeneratedSourceDir       "${myBuildDir}/src-generated/main")
	set(myJavaGeneratedSourceDir   "${myGeneratedSourceDir}/java")
	set(myNativeGeneratedSourceDir "${myGeneratedSourceDir}/native")
	set(myJLibDir                  "${myDir}/data/jlib")
	CreateClasspath(myJavaLibs ${myJLibDir})
	set(myClassPath                ".${PATH_DELIM_H}${myJavaLibs}${PATH_DELIM_H}${myJavaSourceDir}")

	# Used by Java Skirmish AIs
	SetGlobal(SOURCE_ROOT_${myName}_AIINTERFACE      "${myDir}")
	SetGlobal(BUILD_ROOT_${myName}_AIINTERFACE       "${myBuildDir}")
	SetGlobal(${myName}_AIINTERFACE_JAR_BIN          "${myBuildDir}/${myBinJarFile}")
	SetGlobal(${myName}_AIINTERFACE_JAR_SRC          "${myBuildDir}/${mySrcJarFile}")
	SetGlobal(${myName}_AIINTERFACE_POM              "${myBuildDir}/pom-generated.xml")
	SetGlobal(CLASSPATH_${myName}_AIINTERFACE        "${myJavaLibs}${PATH_DELIM_H}${myBuildDir}/${myBinJarFile}")
	SetGlobal(JAVA_SRC_DIR_${myName}_AIINTERFACE     "${myJavaSourceDir}")
	SetGlobal(JAVA_GEN_SRC_DIR_${myName}_AIINTERFACE "${myJavaGeneratedSourceDir}")


	# Generate sources
	# ----------------

	set(commonAwkScriptArgs
		"-v" "SPRING_SOURCE_DIR=${springSourceDir}"
		"-v" "INTERFACE_SOURCE_DIR=${myJavaSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myGeneratedSourceDir}"
		"-v" "NATIVE_GENERATED_SOURCE_DIR=${myNativeGeneratedSourceDir}"
		"-v" "JAVA_GENERATED_SOURCE_DIR=${myJavaGeneratedSourceDir}"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		)

	# A CMake Custom Target will allways be built.
	# from CMake docu:
	# "add_custom_target is ALWAYS CONSIDERED OUT OF DATE"

	# Stub file for dependency tracking
	set(myGeneratedSourceDirStubFile "${CMAKE_CURRENT_BINARY_DIR}/myGeneratedSourceDir.stub")
	set_source_files_properties(${myGeneratedSourceDirStubFile} PROPERTIES GENERATED TRUE)


	# source file lists (static and generated)

	set(myNativeSources
		"${myNativeSourceDir}/InterfaceExport.c"
		"${myNativeSourceDir}/JavaBridge.c"
		"${myNativeSourceDir}/JniUtil.c"
		"${myNativeSourceDir}/JvmLocater_common.c"
		"${myNativeSourceDir}/JvmLocater_linux.c"
		"${myNativeSourceDir}/JvmLocater_windows.c"
		)
	set(myNativeGeneratedSources
		"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.c"
		"${myNativeGeneratedSourceDir}/CallbackJNIBridge.c"
		"${myNativeGeneratedSourceDir}/EventsJNIBridge.c"
		)
	set(myNativeGeneratedHeaders
		"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.h"
		"${myNativeGeneratedSourceDir}/CallbackJNIBridge.h"
		"${myNativeGeneratedSourceDir}/EventsJNIBridge.h"
		)

	set(myJavaSources
		"${myJavaSourceDir}/${myPkg}/Util.java"
		)
	set(myJavaGeneratedSources
		"${myJavaGeneratedSourceDir}/${myPkg}/AI.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/AbstractAI.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/AICallback.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/JniAICallback.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/Enumerations.java"
		)

	set(myGeneratedSources
		${myNativeGeneratedSources}
		${myNativeGeneratedHeaders}
		${myJavaGeneratedSources}
		)
	set_source_files_properties(${myGeneratedSources} PROPERTIES GENERATED TRUE)


	# remove all files in the generates sources dir,
	# that are not generated sources (of this build)
	file(GLOB_RECURSE allGeneratedSrcFiles "${myGeneratedSourceDir}/*")
	foreach    (genFile ${allGeneratedSrcFiles})
		list(FIND myGeneratedSources "${genFile}" isInList)
		if    (${isInList} EQUAL -1)
			file(REMOVE "${genFile}")
		endif ()
	endforeach (genFile)

	#cleanup generated dir if some input file changed
	add_custom_command(
		OUTPUT
			${myGeneratedSourceDirStubFile}
		COMMAND "${CMAKE_COMMAND}"
			"-E" "remove_directory" "${myGeneratedSourceDir}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory"   "${myNativeGeneratedSourceDir}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory"   "${myJavaGeneratedSourceDir}/${myPkg}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "touch" "${myGeneratedSourceDirStubFile}"
		DEPENDS
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/native_wrappCallback.awk"
			"${myAwkScriptsDir}/native_wrappCommands.awk"
			"${myAwkScriptsDir}/jni_wrappCallback.awk"
			"${myAwkScriptsDir}/jni_wrappCommands.awk"
			"${myAwkScriptsDir}/jni_wrappEvents.awk"
			"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
			"${springAIInterfaceSourceDir}/AISCommands.h"
			"${springAIInterfaceSourceDir}/AISEvents.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Cleanup & create generated source directories" VERBATIM
		)

	# 1. & 2. Wrapp Callback (native->native)
	add_custom_command(
		OUTPUT
			"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.h"
			"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.c"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/native_wrappCallback.awk"
			"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/native_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		DEPENDS
			"${myGeneratedSourceDirStubFile}"
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/native_wrappCallback.awk"
			"${myAwkScriptsDir}/native_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating native callback wrapper sources" VERBATIM
		)

	# 3. Wrapp AI Callback (native-JNI->Java)
	add_custom_command(
		OUTPUT
			"${myNativeGeneratedSourceDir}/CallbackJNIBridge.h"
			"${myNativeGeneratedSourceDir}/CallbackJNIBridge.c"
			"${myJavaGeneratedSourceDir}/${myPkg}/AICallback.java"
			"${myJavaGeneratedSourceDir}/${myPkg}/JniAICallback.java"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/jni_wrappCallback.awk"
			"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.h"
		DEPENDS
			"${myGeneratedSourceDirStubFile}"
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/jni_wrappCallback.awk"
			"${myNativeGeneratedSourceDir}/CallbackFunctionPointerBridge.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating JNI callback wrapper sources" VERBATIM
		)

	# 4. Wrapp AI Events (native-JNI->Java)
	add_custom_command(
		OUTPUT
			"${myNativeGeneratedSourceDir}/EventsJNIBridge.h"
			"${myNativeGeneratedSourceDir}/EventsJNIBridge.c"
			"${myJavaGeneratedSourceDir}/${myPkg}/AI.java"
			"${myJavaGeneratedSourceDir}/${myPkg}/AbstractAI.java"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/jni_wrappEvents.awk"
			"${springAIInterfaceSourceDir}/AISEvents.h"
		DEPENDS
			"${myGeneratedSourceDirStubFile}"
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/jni_wrappEvents.awk"
			"${springAIInterfaceSourceDir}/AISEvents.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating JNI events wrapper sources" VERBATIM
		)
	# 5. Wrapp ENUMS

	add_custom_command(
		OUTPUT
			"${myJavaGeneratedSourceDir}/${myPkg}/Enumerations.java"
		COMMAND "${AWK_BIN}"
			${commonAwkScriptArgs}
			"-f" "${myAwkScriptsDir}/jni_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		DEPENDS
			"${myGeneratedSourceDirStubFile}"
			"${commonAwkScriptsDir}/common.awk"
			"${commonAwkScriptsDir}/commonDoc.awk"
			"${myAwkScriptsDir}/jni_wrappCommands.awk"
			"${springAIInterfaceSourceDir}/AISCommands.h"
		WORKING_DIRECTORY
			"${myAwkScriptsDir}"
		COMMENT
			"  ${myTarget}: Generating enums wrapper sources" VERBATIM
		)
	add_custom_target(${myGenTarget} DEPENDS ${myGeneratedSources})
	add_dependencies(generateSources ${myGenTarget})

	# Locate the manifest file
	FindManifestFile("${myDir}" myManifestFile)
	if    (myManifestFile)
		set(myBinJarArgs "cmf" "${myManifestFile}")
	else  (myManifestFile)
		set(myBinJarArgs "cf")
	endif (myManifestFile)

	# Build the native part
	# ---------------------
	if    (MINGW)
		# It is important that this is used instead of the one
		# from the installed JDK, as the jni_md.h is in here too,
		# and this file contains OS (win32) specific information.
		include_directories(BEFORE ${MINGWLIBS}/include/java)
	endif (MINGW)
	include_directories(BEFORE "${rts}/lib/streflop" "${myNativeSourceDir}" "${myNativeGeneratedSourceDir}")
	add_library(${myNativeTarget} MODULE ${myNativeSources} ${myNativeGeneratedSources} ${ai_common_SRC} ${myVersionDepFile})
	Add_Dependencies(${myNativeTarget} generateVersionFiles)
	target_link_libraries(${myNativeTarget} CUtils streflop)
	set_target_properties(${myNativeTarget} PROPERTIES COMPILE_FLAGS "-DUSING_STREFLOP")
	set_target_properties(${myNativeTarget} PROPERTIES OUTPUT_NAME   "AIInterface")
	FixLibName(${myNativeTarget})


	# Build the java part
	# -------------------

	# Write list of source files to an arg-file
	set(mySrcArgFile "${myBuildDir}/sourceFiles.txt")
	if    (EXISTS "${mySrcArgFile}")
		file(REMOVE "${mySrcArgFile}")
	endif (EXISTS "${mySrcArgFile}")
	foreach    (srcFile ${myJavaSources} ${myJavaGeneratedSources})
		file(APPEND "${mySrcArgFile}" "\"${srcFile}\"\n")
	endforeach (srcFile)

	add_custom_command(
		OUTPUT
			"${myBuildDir}/${myBinJarFile}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "remove_directory" "${myJavaBuildDir}"
		COMMAND "${CMAKE_COMMAND}"
			"-E" "make_directory" "${myJavaBuildDir}"
		COMMAND "${Java_JAVAC_EXECUTABLE}" ARGS
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-Xlint:deprecation"
			"-cp" "${myClassPath}"
			"-d" "${myJavaBuildDir}"
			"@${mySrcArgFile}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			${myBinJarArgs} "${myBuildDir}/${myBinJarFile}"
			"-C" "${myJavaBuildDir}" "${myPkgFirstPart}"
		DEPENDS
			${myJavaSources}
			# Using the native target reduces paralellism slightly
			# (less then 1s per build => negliable), but prevetns multiple
			# execution of the source generating custom-commands
			# (which results in wrongly generated source files with "make -j <X>")
			${myJavaGeneratedSources}
			${myNativeTarget}
		WORKING_DIRECTORY
			"${myJavaGeneratedSourceDir}"
		COMMENT
			"  ${myTarget}: Compiling sources and packing library ${myBinJarFile}" VERBATIM
		)

	add_custom_command(
		OUTPUT
			"${myBuildDir}/${mySrcJarFile}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"cf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${myJavaSourceDir}"          "${myPkgFirstPart}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"uf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${myJavaGeneratedSourceDir}" "${myPkgFirstPart}"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"uf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${mySourceDir}"              "native"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"uf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${myGeneratedSourceDir}"     "native"
		COMMAND "${Java_JAR_EXECUTABLE}" ARGS
			"uf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${myDir}"                    "VERSION"
		DEPENDS
			${myJavaSources}
			# Using the native target reduces paralellism slightly
			# (less then 1s per build => negliable), but prevetns multiple
			# execution of the source generating custom-commands
			# (which results in wrongly generated source files with "make -j <X>")
			${myJavaGeneratedSources}
			${myNativeTarget}
		WORKING_DIRECTORY
			"${myBuildDir}"
		COMMENT
			"  ${myTarget}: Creating sources archive ${mySrcJarFile}" VERBATIM
		)

	add_custom_target(${myJavaTarget}
		DEPENDS
			"${myBuildDir}/${myBinJarFile}"
			"${myBuildDir}/${mySrcJarFile}"
		)

	add_custom_target(${myTarget} ALL)
	add_dependencies(${myTarget} ${myNativeTarget})

	# this sets the version in pom.xml
	set(myMavenProperties "-Dmy.version=${myVersion}")
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

	add_dependencies(${myTarget} ${myJavaTarget})

	# Install the native library
	install(TARGETS   ${myNativeTarget} DESTINATION ${myInstLibsDir})
	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION ${myInstLibsDir} FILES_MATCHING PATTERN REGEX "InterfaceInfo\\.lua$")
	install(DIRECTORY "${myDir}/data/" DESTINATION ${myInstDataDir} FILES_MATCHING PATTERN REGEX "InterfaceInfo\\.lua$" EXCLUDE PATTERN "*")
	# Install the library
	install(FILES     "${myBuildDir}/${myBinJarFile}"  DESTINATION ${myInstDataDir})
	# Install the sources archive
	install(FILES     "${myBuildDir}/${mySrcJarFile}"  DESTINATION ${myInstDataDir}/jlib)
endif (BUILD_${myName}_AIINTERFACE)

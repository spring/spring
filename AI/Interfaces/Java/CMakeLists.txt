### Java AI Interface
#
# Global variables set in this file:
# * BUILD_Java_AIINTERFACE
# * Java_AIINTERFACE_VERS
# * Java_AIINTERFACE_TARGET
#
# Functions and macros defined in this file:
# * GetFirstSubDirName
# * CreateClasspath
# * ConcatClasspaths
#


################################################################################
### BEGINN: MACROS_AND_FUNCTIONS
# Define macros and functions to be used in this file and by Java Skirmish AIs

# Returns the name of the first sub-dir (in alphabetical descending order)
# under dir.
macro    (GetFirstSubDirName name_var dir)
	file(GLOB dirContent RELATIVE "${dir}" FOLLOW_SYMLINKS "${dir}/*")
	foreach    (dirPart ${dirContent})
		if    (IS_DIRECTORY "${dir}/${dirPart}")
			set(${name_var} ${dirPart})
			break()
		endif (IS_DIRECTORY "${dir}/${dirPart}")
	endforeach (dirPart)
endmacro (GetFirstSubDirName name_var dir)


# Recursively lists all JAR files in a given directory
# and concatenates them in a Java Classpath compatible way into a single string.
macro    (CreateClasspath classPath_var dir)
	file(GLOB_RECURSE ${classPath_var} FOLLOW_SYMLINKS "${dir}/*.jar")
	# Make sure we use the correct path delimitter for the compiling system
	string(REPLACE ";" "${PATH_DELIM}" ${classPath_var} "${${classPath_var}}")
endmacro (CreateClasspath classPath_var dir)


# Concatenates an arbritrary number of Java ClassPaths (may be empty).
function    (ConcatClasspaths resultingCP_var)
	set(${resultingCP_var} "")
	foreach    (cpPart ${ARGN})
		set(${resultingCP_var} "${${resultingCP_var}}${cpPart}${PATH_DELIM}")
	endforeach (cpPart)
	string(REGEX REPLACE "${PATH_DELIM}\$" "" ${resultingCP_var} "${${resultingCP_var}}")
	set(${resultingCP_var} "${${resultingCP_var}}" PARENT_SCOPE)
endfunction (ConcatClasspaths)

### END: MACROS_AND_FUNCTIONS
################################################################################


set(myName            "Java")
set(myDir             "${CMAKE_CURRENT_SOURCE_DIR}")
set(myNativeSourceDir "${myDir}/src/native")
set(myJavaSourceDir   "${myDir}/src/java")
set(myPkgFirstPart    "com")
set(myPkg             "${myPkgFirstPart}/springrts/ai")


# Check if the user wants to compile the interface
if    ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "JAVA")
	set(AIINTERFACES_JAVA TRUE)
else  ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "JAVA")
	set(AIINTERFACES_JAVA FALSE)
endif ("${AIINTERFACES}" STREQUAL "ALL" OR "${AIINTERFACES}" STREQUAL "JAVA")


# Look for dependencies, but only if the user wants to build the interface
if    (AIINTERFACES_JAVA)
	if    (NOT JAVA_FOUND)
		set(JAVA_FIND_QUIETLY TRUE)
		FIND_PACKAGE(JavaExtended) # Note: this is not the CMake built-in FindJava
	endif (NOT JAVA_FOUND)
	if    (MINGW)
		set (JNI_FOUND TRUE)
	else  (MINGW)
		# this hack is needed for FindJNI.cmake to use the JDK we want it to use,
		# as otherwise it might not find one at all (eg. in the case of OpenJDK)
		if    ( NOT ENV{JAVA_HOME} AND JAVA_HOME )
			SET(ENV{JAVA_HOME} "${JAVA_HOME}")
		endif ( NOT ENV{JAVA_HOME} AND JAVA_HOME )
		FIND_PACKAGE(JNI)
		if (JAVA_INCLUDE_PATH)
			set (JNI_FOUND TRUE)
			include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} ${JNI_INCLUDE_DIRS})
		else (JAVA_INCLUDE_PATH)
			set (JNI_FOUND FALSE)
			message ("warning: No Java includes found!")
		endif (JAVA_INCLUDE_PATH)
	endif (MINGW)
endif (AIINTERFACES_JAVA)

# Check if dependencies of the interface are met
if    (AIINTERFACES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin)
	SetGlobal(BUILD_${myName}_AIINTERFACE TRUE)
else  (AIINTERFACES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin)
	SetGlobal(BUILD_${myName}_AIINTERFACE FALSE)
	message("warning: Java AI Interface will not be built!")
endif (AIINTERFACES_JAVA AND JNI_FOUND AND JAVA_FOUND AND EXISTS ${myDir} AND EXISTS ${myDir}/bin)

# Build
if    (BUILD_${myName}_AIINTERFACE)
	GetVersionFromFile(myVersion ${myDir}/VERSION)
	set(myTarget  "${myName}-AI-Interface")
	set(myDataDir ${AI_INTERFACES_DATA}/${myName}/${myVersion})

	AIInterfaceMessage(STATUS "Found ${myTarget} ${myVersion}")

	SetGlobal(${myName}_AIINTERFACE_VERS   ${myVersion})
	SetGlobal(${myName}_AIINTERFACE_TARGET ${myTarget})

	# build the native part
	aux_source_directory(${myNativeSourceDir} myNativeSources)
	if    (MINGW)
		# It is important that this is used instead of the one
		# from the installed JDK, as the jni_md.h is in here too,
		# and this file contians OS (win32) specific information.
		include_directories(BEFORE ${MINGWLIBS}/include/java)
	endif (MINGW)
	include_directories(BEFORE ${rts}/lib/streflop)
	add_library(${myTarget} MODULE ${myNativeSources} ${ai_common_SRC} ${CUtils_SRC})
	install(TARGETS ${myTarget} DESTINATION ${myDataDir})
	target_link_libraries(${myTarget} streflop)
	set_target_properties(${myTarget} PROPERTIES COMPILE_FLAGS "-DUSING_STREFLOP")
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME   "AIInterface")

	# build the Java part
	if    (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "${MINGWLIBS}/bin/awk.exe")
	else  (CMAKE_HOST_WIN32)
		set(AWK_COMMAND "awk")
	endif (CMAKE_HOST_WIN32)
	set(myAwkScriptsDir            "${myDir}/bin")
	set(commonAwkScriptsDir        "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myBuildDir                 "${CMAKE_CURRENT_BINARY_DIR}")
	set(springSourceDir            "${PROJECT_SOURCE_DIR}")
	set(springAIInterfaceSourceDir "${springSourceDir}/rts/ExternalAI/Interface")
	set(myJavaBuildDir             "${myBuildDir}/classes")
	set(myJarFile                  "AIInterface")
	set(myBinJarFile               "${myJarFile}.jar")
	set(mySrcJarFile               "${myJarFile}-src.jar")
	set(myJavaGeneratedSourceDir   "${myBuildDir}/generated-java-src")
	set(myJLibDir                  "${myDir}/data/jlib")
	#set(myJavaLibs                 "${myJLibDir}/jna/jna.jar${PATH_DELIM}${myJLibDir}/vecmath.jar")
	CreateClasspath(myJavaLibs ${myJLibDir})
	set(myClassPath                ".${PATH_DELIM}${myJavaLibs}${PATH_DELIM}${myJavaSourceDir}")

	# Used by Java Skirmish AIs
	SetGlobal(CLASSPATH_${myName}_AIINTERFACE "${myJavaLibs}${PATH_DELIM}${myBuildDir}/${myBinJarFile}")

	# We have to create these directories, cause otherwise the AWK scripts
	# for generating the source code will not work
	file(MAKE_DIRECTORY "${myJavaGeneratedSourceDir}/${myPkg}/event")
	file(MAKE_DIRECTORY "${myJavaGeneratedSourceDir}/${myPkg}/command")
	file(MAKE_DIRECTORY "${myJavaGeneratedSourceDir}/${myPkg}/oo")
	# needed by javac
	file(MAKE_DIRECTORY "${myJavaBuildDir}")

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#OUTPUT "${myJavaGeneratedSourceDir}/${myPkg}/event/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "springSourceDir=${springSourceDir}"
		"-v" "INTERFACE_SOURCE_DIR=${myJavaSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myJavaGeneratedSourceDir}"
		"-f" "${myAwkScriptsDir}/jna_wrappEvents.awk"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		"${springAIInterfaceSourceDir}/AISEvents.h"
		WORKING_DIRECTORY "${myAwkScriptsDir}"
		COMMENT "  Generating Java AI Event source files ..." VERBATIM)

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#OUTPUT "${myJavaGeneratedSourceDir}/${myPkg}/command/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "springSourceDir=${springSourceDir}"
		"-v" "INTERFACE_SOURCE_DIR=${myJavaSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myJavaGeneratedSourceDir}"
		"-f" "${myAwkScriptsDir}/jna_wrappCommands.awk"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		"${springAIInterfaceSourceDir}/AISCommands.h"
		WORKING_DIRECTORY "${myAwkScriptsDir}"
		COMMENT "  Generating Java AI Command source files ..." VERBATIM)

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#OUTPUT "${myJavaGeneratedSourceDir}/${myPkg}/AICallback.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "springSourceDir=${springSourceDir}"
		"-v" "INTERFACE_SOURCE_DIR=${myJavaSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myJavaGeneratedSourceDir}"
		"-f" "${myAwkScriptsDir}/jna_wrappCallback.awk"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		"${springAIInterfaceSourceDir}/SSkirmishAICallback.h"
		WORKING_DIRECTORY "${myAwkScriptsDir}"
		COMMENT "  Generating Java AI Callback source files ..." VERBATIM)

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/AICallback.java"
		#OUTPUT "${myJavaGeneratedSourceDir}/${myPkg}/oo/*.java"
		COMMAND "${AWK_COMMAND}" ARGS
		"-v" "springSourceDir=${springSourceDir}"
		"-v" "INTERFACE_SOURCE_DIR=${myJavaSourceDir}"
		"-v" "GENERATED_SOURCE_DIR=${myJavaGeneratedSourceDir}"
		"-f" "${myAwkScriptsDir}/java_wrappCallbackOO.awk"
		"-f" "${commonAwkScriptsDir}/common.awk"
		"-f" "${commonAwkScriptsDir}/commonDoc.awk"
		"-f" "${commonAwkScriptsDir}/commonOOCallback.awk"
		"${myJavaGeneratedSourceDir}/${myPkg}/AICallback.java"
		WORKING_DIRECTORY "${myAwkScriptsDir}"
		COMMENT "  Generating Java AI OO Callback source files ..." VERBATIM)

	# We can not auto generate this list,
	# as these files do not yet exist during configure stage
	# The files in this list have to depend on all other hand-written
	# and generated source files of the Java interface
	set(JAVA_GENERATED_SOURCES
		"${myJavaGeneratedSourceDir}/${myPkg}/oo/OOAICallback.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/oo/OOAIFactory.java"
		"${myJavaGeneratedSourceDir}/${myPkg}/oo/AbstractOOAI.java")

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#OUTPUT "${myJavaBuildDir}/*.class"
		DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/*.java"
		DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/event/*.java"
		DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/command/*.java"
		DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/oo/*.java"
		COMMAND "${JAVA_COMPILE}" ARGS
		"${JAVA_COMPILE_FLAG_CONDITIONAL}"
		"-cp" "${myClassPath}"
		"-d" "${myJavaBuildDir}"
		${JAVA_GENERATED_SOURCES}
		WORKING_DIRECTORY "${myJavaGeneratedSourceDir}"
		COMMENT "  Compiling Java sources ..." VERBATIM)

	add_custom_command(TARGET ${myTarget} POST_BUILD
		#DEPENDS "${myJavaBuildDir}/*.class"
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"cmf" "${myJavaSourceDir}/manifest.mf" "${myBuildDir}/${myBinJarFile}"
		"-C" "${myJavaBuildDir}" "${myPkgFirstPart}"
		WORKING_DIRECTORY "${myBuildDir}"
		COMMENT "  Creating Java interface library ${myBinJarFile} ..." VERBATIM)

	add_custom_command(TARGET ${myTarget} POST_BUILD
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"cf" "${myBuildDir}/${mySrcJarFile}"
		"-C" "${myJavaSourceDir}" "${myPkgFirstPart}"
		WORKING_DIRECTORY "${myBuildDir}"
		COMMENT "  Creating Java interface source archive ${mySrcJarFile} ..." VERBATIM)
	add_custom_command(TARGET ${myTarget} POST_BUILD
		#DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/event/*.java"
		#DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/command/*.java"
		#DEPENDS "${myJavaGeneratedSourceDir}/${myPkg}/oo/*.java"
		COMMAND "${JAVA_ARCHIVE}" ARGS
		"uf" "${myBuildDir}/${mySrcJarFile}"
		"-C" "${myJavaGeneratedSourceDir}" "${myPkgFirstPart}"
		WORKING_DIRECTORY "${myBuildDir}"
		COMMENT "  Updating Java interface source archive ${mySrcJarFile} with generated sources ..." VERBATIM)

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION ${myDataDir})
	# Install the library
	install(FILES     "${myBuildDir}/${myBinJarFile}"  DESTINATION ${myDataDir})
	# Install the sources archive
	install(FILES     "${myBuildDir}/${mySrcJarFile}"  DESTINATION ${myDataDir}/jlib)
endif (BUILD_${myName}_AIINTERFACE)

### NullJavaAI Skirmish AI
#
# BUILD_Java_AIINTERFACE is set in ../../Interfaces/Java/CMakeLists.txt
#

# AI specific properties
set(myName                   "NullJavaAI")
set(myDir                    "${CMAKE_CURRENT_SOURCE_DIR}")
set(mySourceDir              "${myDir}/src")
set(myJavaPkgFirstPart       "nulljavaai")
set(myAdditionalClassPath    "")
#set(myJLibDir                "${myDir}/data/jlib")
#set(myAdditionalClassPath    "${PATH_DELIM}${myJLibDir}/myOwnLib.jar")


# Check if the user wants to compile the AI
if    (BUILD_Java_AIINTERFACE)
	set(BUILD_THIS_SKIRMISHAI TRUE)
else  (BUILD_Java_AIINTERFACE)
	set(BUILD_THIS_SKIRMISHAI FALSE)
	message("warning: ${myName} Skirmish AI will not be built!")
endif (BUILD_Java_AIINTERFACE)


# Compile and install
if    (BUILD_THIS_SKIRMISHAI)
	# Assemble meta data
	GetVersionFromFile(myVersion "${myDir}/VERSION")
	set(myTarget        "${myName}")
	set(mySourceTarget  "${myTarget}-SRC")
	set(myDataDir       "${SKIRMISH_DATA}/${myName}/${myVersion}")
	# CMAKE_CURRENT_BINARY_DIR: .../spring-build-dir/AI/Skirmish/${myName}
	set(myBuildDir      "${CMAKE_CURRENT_BINARY_DIR}")
	set(myJavaBuildDir  "${myBuildDir}/classes")
	set(myJarFile       "SkirmishAI")
	set(myBinJarFile    "${myJarFile}.jar")
	set(mySrcJarFile    "${myJarFile}-src.jar")
	SkirmishAIMessage(STATUS "Found Skirmish AI: ${myName} ${myVersion}")

	# Create a list of all the AIs source files
	file(GLOB_RECURSE mySources RELATIVE "${mySourceDir}" FOLLOW_SYMLINKS "${mySourceDir}/*.java")

	# Compile the library
	# needed by javac
	file(MAKE_DIRECTORY "${myJavaBuildDir}")
	#add_custom_command(OUTPUT "${myDataDir}/${myBinJarFile}"
	#add_custom_command(TARGET ${myTarget}
	add_custom_target(${myTarget} ALL
		DEPENDS ${Java_AIINTERFACE_TARGET}
		COMMAND "${JAVA_COMPILE}"
			"${JAVA_COMPILE_FLAG_CONDITIONAL}"
			"-cp" "${CLASSPATH_Java_AIINTERFACE}${myAdditionalClassPath}"
			"-d" "${myJavaBuildDir}"
			${mySources}
		COMMAND "${JAVA_ARCHIVE}"
			"cmf" "${myDir}/manifest.mf" "${myBuildDir}/${myBinJarFile}"
			"-C" "${myJavaBuildDir}" "${myJavaPkgFirstPart}"
		WORKING_DIRECTORY "${mySourceDir}"
		COMMENT "  Creating library ${myBinJarFile}" VERBATIM)

	# Pack the sources
	add_custom_target(${mySourceTarget} ALL
		COMMAND "${JAVA_ARCHIVE}"
			"cf" "${myBuildDir}/${mySrcJarFile}"
			"-C" "${mySourceDir}" "${myJavaPkgFirstPart}"
		WORKING_DIRECTORY "${mySourceDir}"
		COMMENT "  Creating sources archive ${mySrcJarFile}" VERBATIM)

	# Install the data files
	install(DIRECTORY "${myDir}/data/" DESTINATION "${myDataDir}")
	# Install the library
	install (FILES ${myBuildDir}/${myBinJarFile} DESTINATION ${myDataDir})
	# Install the sources archive
	install (FILES ${myBuildDir}/${mySrcJarFile} DESTINATION ${myDataDir}/jlib)
endif (BUILD_THIS_SKIRMISHAI)

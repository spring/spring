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
	set(myBinDir "${myDir}/bin")
	set(commonBinDir "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(myGeneratedSourceDir "${CMAKE_CURRENT_BINARY_DIR}/generated-src")
	set(springSourceDir            "${PROJECT_SOURCE_DIR}")
	set(springAIInterfaceSourceDir "${springSourceDir}/rts/ExternalAI/Interface")

	SetGlobal(${myName}_AIWRAPPER_GENERATED_SRC_DIR "${myGeneratedSourceDir}")

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
		OUTPUT "${myGeneratedSourceDir}/*.cpp"
		COMMAND ${myGenerateCommand}
		WORKING_DIRECTORY "${myBinDir}"
		COMMENT "  Generating C++ AI Wrapper Callback source files" VERBATIM)
	ADD_CUSTOM_TARGET(generate_mySources DEPENDS "${myGeneratedSourceDir}/*.cpp")

	# this will generate the sources on "cmake configure", which is needed
	# for cmake to be able to list the source files
	EXECUTE_PROCESS(
		COMMAND ${myGenerateCommand}
		WORKING_DIRECTORY "${myBinDir}")

	# build the static lib
	aux_source_directory(${myGeneratedSourceDir} mySources)
	list (APPEND mySources ${ai_common})
	add_library(${myTarget} STATIC ${mySources})
	set_target_properties(${myTarget} PROPERTIES OUTPUT_NAME "${myName}")
	SET_SOURCE_FILES_PROPERTIES("${myGeneratedSourceDir}/*.cpp" PROPERTIES GENERATED 1)
	ADD_DEPENDENCIES(${myTarget} generate_mySources)
endif (BUILD_${myName}_AIWRAPPER)

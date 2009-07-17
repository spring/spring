### AI
#
# Global variables set in this file:
# * AIDIR
#
# Functions and macros defined in this file:
# * SetGlobal
# * GetListOfSubModules
# * GetVersionFromFile
# * GetLastPathPart
#

add_definitions(-DBUILDING_AI)

set(AIDIR           ${DATADIR} CACHE STRING "Where AIs are installed")
set(rts             "${CMAKE_SOURCE_DIR}/rts")
set(AI_FIND_QUIETLY FALSE)

include_directories(
	${rts}
	${rts}/System
	${rts}/ExternalAI/Interface
	Wrappers
	${SDL_INCLUDE_DIR})


# Set these for Interfaces and AIs with C sources
set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2}")
set(CMAKE_C_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3}")
set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if    (WIN32)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
endif (WIN32)


if ("${AITYPES}" STREQUAL "ALL" OR "${AITYPES}" STREQUAL "NATIVE")
	set(AITYPES_NATIVE "TRUE")
endif ("${AITYPES}" STREQUAL "ALL" OR "${AITYPES}" STREQUAL "NATIVE")
if ("${AITYPES}" STREQUAL "ALL" OR "${AITYPES}" STREQUAL "JAVA")
	set(AITYPES_JAVA "TRUE")
endif ("${AITYPES}" STREQUAL "ALL" OR "${AITYPES}" STREQUAL "JAVA")

IF (CMAKE_HOST_WIN32)
	set(AWK_COMMAND "${MINGWLIBS}/bin/awk.exe")
ELSE (CMAKE_HOST_WIN32)
	set(AWK_COMMAND "awk")
ENDIF(CMAKE_HOST_WIN32)


# Assemble common additional native AI sources
list(APPEND ai_common_SRC ${rts}/Game/GameVersion)

# Assemble the CUtils sources
aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils CUtils_SRC)


IF (AITYPES_NATIVE)
	# Assemble the Legacy C++ AI sources
	aux_source_directory("Wrappers/LegacyCpp"          legacyCppAiSrcFiles)
	list(APPEND legacyCppAiSrcFiles ${ai_common_SRC})
	list(APPEND legacyCppAiSrcFiles ${rts}/System/float3)
	list(APPEND legacyCppAiSrcFiles ${rts}/Sim/Misc/DamageArray)

	# Assemble the creg sources
	aux_source_directory("${rts}/System/creg"          creg)

	# Compile the Legacy C++ AI wrapper static library
	add_library(legacycpp      STATIC ${legacyCppAiSrcFiles})

	# Compile the Legacy C++ AI wrapper static library with creg support
	add_library(legacycpp-creg STATIC ${legacyCppAiSrcFiles} ${creg})
	set_target_properties(legacycpp-creg PROPERTIES COMPILE_FLAGS "-DUSING_CREG")


	# Build static library for (new) C++ AIs
	set(CPPWRAPPER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Wrappers/Cpp")
	set(CPPWRAPPER_AWK_SCRIPTS_DIR "${CPPWRAPPER_SOURCE_DIR}/bin")
	set(AWK_COMMON_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils/bin")
	set(CPPWRAPPER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/Wrappers/Cpp")
	set(CPPWRAPPER_GENERATED_SRC_DIR "${CPPWRAPPER_BUILD_DIR}/generated-src")
	set(SPRING_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
	set(SPRING_AIINTERFACE_SOURCE_DIR "${SPRING_SOURCE_DIR}/rts/ExternalAI/Interface")

	file(MAKE_DIRECTORY "${CPPWRAPPER_GENERATED_SRC_DIR}")

	# generate the source files
	SET(CPPWRAPPER_GENERATED_COMMAND
		"${AWK_COMMAND}"
		"-v" "SPRING_SOURCE_DIR=${SPRING_SOURCE_DIR}"
		"-v" "GENERATED_SOURCE_DIR=${CPPWRAPPER_GENERATED_SRC_DIR}"
		"-f" "${CPPWRAPPER_AWK_SCRIPTS_DIR}/wrappCallback.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/common.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonDoc.awk"
		"-f" "${AWK_COMMON_SCRIPTS_DIR}/commonOOCallback.awk"
		"${SPRING_AIINTERFACE_SOURCE_DIR}/SSkirmishAICallback.h")

	# this will regenerate the sources whenever the DEPENDS changes
	ADD_CUSTOM_COMMAND(
		DEPENDS "${SPRING_AIINTERFACE_SOURCE_DIR}/SSkirmishAICallback.h"
		OUTPUT "${CPPWRAPPER_GENERATED_SRC_DIR}/*.cpp"
		COMMAND ${CPPWRAPPER_GENERATED_COMMAND}
		WORKING_DIRECTORY "${CPPWRAPPER_AWK_SCRIPTS_DIR}"
		COMMENT "  Generating C++ AI Wrapper Callback source files" VERBATIM)
	ADD_CUSTOM_TARGET(generate_cppaifiles DEPENDS "${CPPWRAPPER_GENERATED_SRC_DIR}/*.cpp")

	# this will generate the sources on "cmake configure", which is needed
	# for cmake to be able to list the source files
	EXECUTE_PROCESS(
		COMMAND ${CPPWRAPPER_GENERATED_COMMAND}
		WORKING_DIRECTORY "${CPPWRAPPER_AWK_SCRIPTS_DIR}")

	# build the static lib
	aux_source_directory(${CPPWRAPPER_GENERATED_SRC_DIR} cppaifiles)
	list (APPEND cppaifiles ${ai_common})
	add_library(cppaiwrapper STATIC ${cppaifiles})
	SET_SOURCE_FILES_PROPERTIES("${CPPWRAPPER_GENERATED_SRC_DIR}/*.cpp" PROPERTIES GENERATED 1)
	ADD_DEPENDENCIES(cppaiwrapper generate_cppaifiles)
ENDIF(AITYPES_NATIVE)


################################################################################
### BEGINN: MACROS_AND_FUNCTIONS
# Define macros and functions to be used in this file and by Java Skirmish AIs

# Sets a variable in global scope
function    (SetGlobal var value)
	set(${var} "${value}" CACHE INTERNAL "" FORCE)
endfunction (SetGlobal)


# Find all CMakeLists.txt files in sub-directories
macro    (GetListOfSubModules list_var)
	file(GLOB ${list_var} RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	# Strip away the "/CMakeLists.txt" parts, so we end up with just a list of dirs,
	# for example: AAI;RAI;KAIK
	string(REPLACE "//CMakeLists.txt" "" ${list_var} "${${list_var}}")
endmacro (GetListOfSubModules list_var)


# Gets the version from a text file.
# (actually just reads the text file content into a variable)
macro    (GetVersionFromFile vers_var vers_file)
	if    (EXISTS ${vers_file})
		file(STRINGS "${vers_file}" ${vers_var} LIMIT_COUNT 1)
	else  (EXISTS ${vers_file})
		set(${vers_var} "UNKNOWN_VERSION")
	endif (EXISTS ${vers_file})
endmacro (GetVersionFromFile vers_var vers_file)


# Returns the name of the dir or file specified by a path.
# example: "/A/B/C" -> "C"
macro    (GetLastPathPart part_var dir)
	string(REGEX REPLACE ".*[\\/]" "" ${part_var} ${dir})
endmacro (GetLastPathPart part_var dir)

### END: MACROS_AND_FUNCTIONS
################################################################################


Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)

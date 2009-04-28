set (AIDIR ${DATADIR} CACHE STRING "Where AIs are installed")

add_definitions(-DBUILDING_AI)

include_directories(
	${CMAKE_SOURCE_DIR}/rts
	${CMAKE_SOURCE_DIR}/rts/System
	${CMAKE_SOURCE_DIR}/rts/ExternalAI/Interface
	Wrappers
	${SDL_INCLUDE_DIR})


# set these for C Interfaces and AIs
SET(CMAKE_C_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}")
SET(CMAKE_C_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2}")
SET(CMAKE_C_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3}")
SET(CMAKE_C_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
SET(CMAKE_C_FLAGS_PROFILE        "${CMAKE_CXX_FLAGS_PROFILE}")

remove_definitions(-DSTREFLOP_SSE)
add_definitions(${PIC_FLAG} -D_REENTRANT -D_GNU_SOURCE=1)

if (WIN32)
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--kill-at -Wl,--add-stdcall-alias")
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

list (APPEND ai_common ${CMAKE_SOURCE_DIR}/rts/Game/GameVersion)

IF (AITYPES_NATIVE)
	# Build static libraries for Legacy C++ AIs
	aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/LegacyCpp legacycppaifiles)
	list (APPEND legacycppaifiles ${ai_common})
	list (APPEND legacycppaifiles ${CMAKE_SOURCE_DIR}/rts/System/float3)
	list (APPEND legacycppaifiles ${CMAKE_SOURCE_DIR}/rts/Sim/Misc/DamageArray)
	add_library(legacycpp STATIC ${legacycppaifiles})

	aux_source_directory(${CMAKE_SOURCE_DIR}/rts/System/creg creg)
	add_library(legacycpp-creg STATIC ${legacycppaifiles} ${creg})
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

aux_source_directory(${CMAKE_SOURCE_DIR}/AI/Wrappers/CUtils cutilsenv)

Add_Subdirectory(Interfaces)
Add_Subdirectory(Skirmish)

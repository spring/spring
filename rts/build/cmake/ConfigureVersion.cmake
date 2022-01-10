# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

#
# example usage:
#Add_Custom_Command(
#	TARGET
#		configureVersion
#	COMMAND "${CMAKE_COMMAND}"
#		"-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
#		"-DCMAKE_MODULES_SPRING=${CMAKE_MODULES_SPRING}"
#		"-DVERSION_ADDITIONAL=ABC"
#		"-DGENERATE_DIR=${CMAKE_BINARY_DIR}"
#		"-P" "${CMAKE_MODULES_SPRING}/ConfigureFile.cmake"
#	COMMENT
#		"Configure Version files" VERBATIM
#	)
#

cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_SPRING}")

include(UtilVersion)



# Fetch through git or from the VERSION file
fetch_spring_version(${SOURCE_ROOT} SPRING_ENGINE)
parse_spring_version(SPRING_VERSION_ENGINE "${SPRING_ENGINE_VERSION}")

# We define these, so it may be used in the to-be-configured files
set(SPRING_VERSION_ENGINE "${SPRING_ENGINE_VERSION}")
if ("${SPRING_VERSION_ENGINE}" MATCHES "^${VERSION_REGEX_RELEASE}$")
	set(SPRING_VERSION_ENGINE_RELEASE 1)
else ()
	set(SPRING_VERSION_ENGINE_RELEASE 0)
endif ()

# This is supplied by -DVERSION_ADDITIONAL="abc"
set(SPRING_VERSION_ENGINE_ADDITIONAL "${VERSION_ADDITIONAL}")



message ("Spring engine version: ${SPRING_ENGINE_VERSION} (${SPRING_VERSION_ENGINE_ADDITIONAL})")



file(MAKE_DIRECTORY "${GENERATE_DIR}/src-generated/engine/System")
configure_file(
		"${SOURCE_ROOT}/rts/System/VersionGenerated.h.template"
		"${GENERATE_DIR}/src-generated/engine/System/VersionGenerated.h"
		@ONLY
	)

file(MAKE_DIRECTORY "${GENERATE_DIR}")
configure_file(
		"${SOURCE_ROOT}/VERSION.template"
		"${GENERATE_DIR}/VERSION"
		@ONLY
	)

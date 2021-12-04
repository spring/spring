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

Cmake_Minimum_Required(VERSION 2.6)

List(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_SPRING}")

Include(UtilVersion)



# Fetch through git or from the VERSION file
FetchSpringVersion(${SOURCE_ROOT} SPRING_ENGINE)
ParseSpringVersion(SPRING_VERSION_ENGINE "${SPRING_ENGINE_VERSION}")

# We define these, so it may be used in the to-be-configured files
Set(SPRING_VERSION_ENGINE "${SPRING_ENGINE_VERSION}")
If     ("${SPRING_VERSION_ENGINE}" MATCHES "^${VERSION_REGEX_RELEASE}$")
	Set(SPRING_VERSION_ENGINE_RELEASE 1)
Else   ()
	Set(SPRING_VERSION_ENGINE_RELEASE 0)
EndIf  ()

# This is supplied by -DVERSION_ADDITIONAL="abc"
Set(SPRING_VERSION_ENGINE_ADDITIONAL "${VERSION_ADDITIONAL}")



Message("Spring engine version: ${SPRING_ENGINE_VERSION} (${SPRING_VERSION_ENGINE_ADDITIONAL})")



File(MAKE_DIRECTORY "${GENERATE_DIR}/src-generated/engine/System")
Configure_File(
		"${SOURCE_ROOT}/rts/System/VersionGenerated.h.template"
		"${GENERATE_DIR}/src-generated/engine/System/VersionGenerated.h"
		@ONLY
	)

File(MAKE_DIRECTORY "${GENERATE_DIR}")
Configure_File(
		"${SOURCE_ROOT}/VERSION.template"
		"${GENERATE_DIR}/VERSION"
		@ONLY
	)




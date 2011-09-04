# - Find Maven binary
# Find the Maven executable
#
#  MVN_BIN         - Maven executable
#  MVN_ATTRIBUTES_DEFAULT - These should always be used as attributes to MVN_BIN
#                    (in addition to others)
#  MVN_VERSION     - Maven version (first line of output of "${MVN_BIN} --version")
#  MAVEN_FOUND     - TRUE if the Maven binary was found.

If     (APPLE)
	# HACK Workaround to prevent maven beeing used on Mac,
	# because CMake fails to replace the version-tags
	# in the generated pom files.
	Set(MAVEN_FOUND FALSE)
Else    (APPLE)

	Include(FindPackageHandleStandardArgs)

	If    (MVN_BIN)
		# Already in cache, be silent
		Set(mvn_FIND_QUIETLY TRUE)
	EndIf (MVN_BIN)

	Find_Program(MVN_BIN NAMES mvn DOC "Maven command line executable")
EndIf   (APPLE)


if    (MVN_BIN)
	# Try to fetch the Maven version
	Execute_Process(COMMAND "${MVN_BIN}" "--version"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE MVN_RET
			OUTPUT_VARIABLE MVN_VERSION
			ERROR_QUIET)
	If    (NOT ${MVN_RET} EQUAL 0)
		Set(MVN_BIN)
		Set(MVN_VERSION)
	EndIf (NOT ${MVN_RET} EQUAL 0)

	If    (MVN_VERSION)
		# Strip away everything but the first line
		String(REGEX REPLACE "\n.*" "" MVN_VERSION "${MVN_VERSION}")
		If    (NOT Maven_FIND_QUIETLY)
			Message(STATUS "Maven version: ${MVN_VERSION}")
		EndIf (NOT Maven_FIND_QUIETLY)

		# Run in non-interactive mode
		Set(MVN_ATTRIBUTES_DEFAULT "--batch-mode" "-DskipTests")
		If    (NOT CMAKE_VERBOSE_MAKEFILE)
			# Quiet output - only show errors
			set(MVN_ATTRIBUTES_DEFAULT ${MVN_ATTRIBUTES_DEFAULT} "--quiet")
		EndIf (NOT CMAKE_VERBOSE_MAKEFILE)
	EndIf (MVN_VERSION)
EndIf (MVN_BIN)

# Handle the QUIETLY and REQUIRED arguments and set MAVEN_FOUND to TRUE
# if MVN_BIN is valid
Find_Package_Handle_Standard_Args(Maven DEFAULT_MSG MVN_BIN MVN_VERSION MVN_ATTRIBUTES_DEFAULT)

# Show these variables only in the advanced view in the GUI, and make them global
MARK_AS_ADVANCED(
	MVN_BIN
	MVN_VERSION
	MVN_ATTRIBUTES_DEFAULT
	MAVEN_FOUND
	)


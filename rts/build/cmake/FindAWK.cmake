# - Find AWK binary
# Find the native AWK executable
#
#  AWK_BIN         - AWK executable
#  AWK_VERSION     - AWK version (first line of output of "${AWK_BIN} -W version")
#  AWK_FOUND       - TRUE if AWK binary was found.

INCLUDE(FindPackageHandleStandardArgs)

IF    (AWK_BIN)
	# Already in cache, be silent
	SET(AWK_FIND_QUIETLY TRUE)
ENDIF (AWK_BIN)


SET(AWK_NAMES awk gawk mawk nawk)

if    (CMAKE_HOST_WIN32)
	set(AWK_BIN "${MINGWLIBS}/bin/awk.exe")
	if    (NOT EXISTS AWK_BIN)
		find_program(AWK_BIN NAMES ${AWK_NAMES})
	endif (NOT EXISTS AWK_BIN)
else  (CMAKE_HOST_WIN32)
	find_program(AWK_BIN NAMES ${AWK_NAMES})
endif (CMAKE_HOST_WIN32)

# Handle the QUIETLY and REQUIRED arguments and set AWK_FOUND to TRUE
# if AWK_BIN is valid
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AWK DEFAULT_MSG AWK_BIN)

if    (AWK_FOUND)
	# try to fetch AWK version
	# We use "-W version" instead of "--version", cause the later is
	# not supported by all AWK versions, for example mawk.
	EXECUTE_PROCESS(COMMAND ${AWK_BIN} -W version
		RESULT_VARIABLE RET_VAL
		OUTPUT_VARIABLE AWK_VERSION
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if    (${RET_VAL} EQUAL 0)
		# reduce to first line
		string(REPLACE "\\n.*" "" AWK_VERSION "${AWK_VERSION}")
		if    (NOT AWK_FIND_QUIETLY)
			message(STATUS "AWK version: ${AWK_VERSION}")
		endif (NOT AWK_FIND_QUIETLY)
	else  (${RET_VAL} EQUAL 0)
		# clear
		set(AWK_VERSION)
	endif (${RET_VAL} EQUAL 0)
else  (AWK_FOUND)
	# clear
	set(AWK_BIN)
	set(AWK_VERSION)
endif (AWK_FOUND)

# Show these variables only in the advanced view in the GUI, and make them global
MARK_AS_ADVANCED(
	AWK_FOUND
	AWK_BIN
	AWK_VERSION
	)

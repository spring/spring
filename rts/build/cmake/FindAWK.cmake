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
	# Try to fetch the AWK version
	#
	# There are different ways of doing this, and not all implementations
	# support all of them. What is worse: none of the ways is supported by all
	# implementations. :/
	#
	# Common Implementations:
	# * GAWK: linux (& windows) standard
	# * MAWK: gentoo
	# * BWK:  OS X & BSDs (announces itsself just as "awk version 20070501")
	# more info: http://en.wikipedia.org/wiki/AWK#Versions_and_implementations
	#
	# a) awk -Wv        (the standard, not supported by BWK)
	# b) awk -W version (less standard, more verbose version of the above)
	# c) awk --version  (works for BWK and GAWK)
	# d) awk -version   (works for BWK)
	#
	# So we first try a) and if output is "", we try c)
	#
	EXECUTE_PROCESS(COMMAND ${AWK_BIN} -Wv
		RESULT_VARIABLE RET_VAL
		OUTPUT_VARIABLE AWK_VERSION
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if    ( NOT${RET_VAL} EQUAL 0 OR AWK_VERSION STREQUAL "")
		EXECUTE_PROCESS(COMMAND ${AWK_BIN} --version
			RESULT_VARIABLE RET_VAL
			OUTPUT_VARIABLE AWK_VERSION
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif ( NOT${RET_VAL} EQUAL 0 OR AWK_VERSION STREQUAL "")

	if    (${RET_VAL} EQUAL 0)
		# reduce to first line
		String(REGEX REPLACE "\n.*" "" AWK_VERSION "${AWK_VERSION}")
		if    (NOT AWK_FIND_QUIETLY)
			message(STATUS "AWK version: ${AWK_VERSION}")
		endif (NOT AWK_FIND_QUIETLY)
	else  (${RET_VAL} EQUAL 0)
		# failed to fetch version, clear
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

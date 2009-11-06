# - Find AWK binary
# Find the native AWK executable
#
#  AWK_BIN         - AWK executable
#  AWK_FOUND       - TRUE if AWK binary was found.


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

if    (NOT AWK_FOUND)
	set(AWK_BIN) # clear
endif (NOT AWK_FOUND)

# Show these variables only in the advanced view in the GUI, and make them global
MARK_AS_ADVANCED(
	AWK_FOUND
	AWK_BIN
	)

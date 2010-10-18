# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find Pandoc
# Find the native Pandoc binary
#
#  PANDOC_BIN   - will be set to the Pandoc executable (eg. pandoc.exe)
#  PANDOC_FOUND - TRUE if Pandoc was found

INCLUDE(FindPackageHandleStandardArgs)

IF    (PANDOC_BIN)
	# Already in cache, be silent
	SET(PANDOC_FIND_QUIETLY TRUE)
ENDIF (PANDOC_BIN)

find_program(PANDOC_BIN
	NAMES pandoc
	HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
	PATH_SUFFIXES bin
	DOC "Pandoc executable"
	)

# handle the QUIETLY and REQUIRED arguments and set PANDOC_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PANDOC DEFAULT_MSG PANDOC_BIN)

MARK_AS_ADVANCED(PANDOC_BIN)

# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find 7zip
# Find the native 7zip binary
#
#  7ZIP_BIN   - will be set to the 7zip executable (eg. 7z.exe)
#  7ZIP_FOUND - TRUE if 7zip was found

IF    (7ZIP_BIN)
	# Already in cache, be silent
	SET(7zip_FIND_QUIETLY TRUE)
ENDIF (7ZIP_BIN)

# 7zr(.exe) only supports 7z archives, while 7z(.exe) and 7za(.exe)
# additionally support many other formats (eg zip)
find_program(7ZIP_BIN
	NAMES 7z 7za
	HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
	PATH_SUFFIXES bin
	DOC "7zip executable"
	)

# handle the QUIETLY and REQUIRED arguments and set 7ZIP_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(7zip DEFAULT_MSG 7ZIP_BIN)

MARK_AS_ADVANCED(7ZIP_BIN)

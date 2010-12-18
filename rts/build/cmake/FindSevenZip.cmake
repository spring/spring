# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find 7zip
# Find the native 7zip binary
#
# NOTE: We can not use 7ZIP* or 7zip* as var name, cause in CMake,
#       var names should not start with numbers.
#
#  SEVENZIP_BIN   - will be set to the 7zip executable (eg. 7z.exe)
#  SEVENZIP_FOUND - TRUE if 7zip was found

INCLUDE(FindPackageHandleStandardArgs)

IF    (SEVENZIP_BIN)
	# Already in cache, be silent
	SET(SevenZip_FIND_QUIETLY TRUE)
ENDIF (SEVENZIP_BIN)

# 7zr(.exe) only supports 7z archives, while 7z(.exe) and 7za(.exe)
# additionally support many other formats (eg zip)
find_program(SEVENZIP_BIN
	NAMES 7z 7za
	HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
	PATH_SUFFIXES bin
	DOC "7zip executable"
	)

# handle the QUIETLY and REQUIRED arguments and set SEVENZIP_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SevenZip DEFAULT_MSG SEVENZIP_BIN)

MARK_AS_ADVANCED(SEVENZIP_BIN)

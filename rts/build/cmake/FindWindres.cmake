# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find windres executable
# manipulate Windows resources
#
#  WINDRES_BIN    - will be set to the windres executable (eg. windres.exe)
#  WINDRES_FOUND  - TRUE if windres was found

# Already in cache, be silent
IF    (WINDRES_BIN)
	SET(Windres_FIND_QUIETLY TRUE)
ENDIF (WINDRES_BIN)



FIND_PROGRAM(WINDRES_BIN
	NAMES
		windres
		i586-mingw32msvc-windres
		i586-pc-mingw32-windres
		i686-mingw32-windres
	DOC "path to mingw's windres executable"
	)

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set WINDRES_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Windres DEFAULT_MSG WINDRES_BIN)

MARK_AS_ADVANCED(WINDRES_BIN)

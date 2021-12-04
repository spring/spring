# Downloaded from: http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/cmake/
# License: GPL v2, http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/COPYING
#
# Modifications:
# 2008.01.16 Tobi Vollebregt -- changed Devil->DEVIL for consistency
#                            -- added devil alternative names for MinGW
#                            -- removed "looking for devil" status message

# - Find DevIL
# Find the native DevIL includes and libraries.
#
#  IL_INCLUDE_DIR  - Where to find "IL/il.h"
#  IL_IL_LIBRARY   - Path to the IL shared library
#  IL_LIBRARIES    - List of libraries when using IL
#  IL_FOUND        - True if IL is found.

INCLUDE(FindPackageHandleStandardArgs)

IF    (IL_INCLUDE_DIR AND IL_IL_LIBRARY)
	# Already in cache, be silent
	SET(DevIL_FIND_QUIETLY TRUE)
ENDIF (IL_INCLUDE_DIR AND IL_IL_LIBRARY)



set(IL_FIND_LIB_STD_ARGS
	PATH_SUFFIXES
		lib64
		lib
		libs64
		libs
		libs/Win32
		libs/Win64
	PATHS
		${PROJECT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}
		$ENV{LD_LIBRARY_PATH}
		$ENV{LIBRARY_PATH}
		/usr
		/usr/local
		/usr/bin
	)

FIND_LIBRARY(IL_IL_LIBRARY
	NAMES
		IL
		devil
		DevIL
	${IL_FIND_LIB_STD_ARGS}
)


SET(IL_LIBRARIES "")
IF    (IL_IL_LIBRARY)
	LIST(APPEND IL_LIBRARIES ${IL_IL_LIBRARY})
ENDIF (IL_IL_LIBRARY)

GET_FILENAME_COMPONENT(IL_LIBRARY_DIR "${IL_IL_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(IL_LIBRARY_SUPER_DIR "${IL_LIBRARY_DIR}" PATH)



set(IL_FIND_HEADER_STD_ARGS
	PATHS
		${PROJECT_BINARY_DIR}/include
		${PROJECT_SOURCE_DIR}/include
		${IL_LIBRARY_SUPER_DIR}/include
		$ENV{CPATH}
		/usr/local/include
		/usr/include
	NO_DEFAULT_PATH
	)

FIND_FILE(IL_IL_HEADER IL/il.h
	${IL_FIND_INCLUDE_STD_ARGS}
)

FIND_PATH(IL_INCLUDE_DIR IL/il.h
	${IL_FIND_INCLUDE_STD_ARGS}
)



# handle the QUIETLY and REQUIRED arguments and set IL_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DevIL DEFAULT_MSG IL_IL_HEADER IL_INCLUDE_DIR IL_IL_LIBRARY IL_LIBRARIES)


MARK_AS_ADVANCED(
	IL_INCLUDE_DIR
	IL_IL_LIBRARY
	IL_LIBRARIES
	)

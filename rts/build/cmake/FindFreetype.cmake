# Downloaded from: http://websvn.kde.org/trunk/KDE/kdelibs/cmake/modules/
# License: see the accompanying COPYING-CMAKE-SCRIPTS file
#
# Modifications:
# 2008.01.16 Tobi Vollebregt -- don't use freetype-config on MinGW
#                            -- special case FREETYPE_INCLUDE_DIR for MinGW

# - Try to find the freetype library
# Once done this will define
#
#  FREETYPE_FOUND - system has Freetype
#  FREETYPE_INCLUDE_DIRS - the FREETYPE include directories
#  FREETYPE_LIBRARIES - Link these to use FREETYPE
#  FREETYPE_INCLUDE_DIR - internal

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

INCLUDE(FindPackageHandleStandardArgs)

if    (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)
	# Already in cache, be silent
	set(Freetype_FIND_QUIETLY TRUE)
endif (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

# only search if not yet specified
if    (NOT FREETYPE_LIBRARIES OR NOT FREETYPE_INCLUDE_DIR)
	find_program(FREETYPECONFIG_EXECUTABLE
		NAMES
			freetype-config
		PATHS
			/opt/local/bin
		)

	# clear vars
	set(FREETYPE_LIBRARIES)
	set(FREETYPE_INCLUDE_DIR)

	# if freetype-config has been found
	if    (FREETYPECONFIG_EXECUTABLE AND NOT MINGW)
		execute_process(COMMAND ${FREETYPECONFIG_EXECUTABLE} --libs
			RESULT_VARIABLE RET_VAL_libs
			OUTPUT_VARIABLE FREETYPE_LIBRARIES
			OUTPUT_STRIP_TRAILING_WHITESPACE
			)
		if    (NOT ${RET_VAL_libs} EQUAL 0)
			# clear
			set(FREETYPE_LIBRARIES)
		endif (NOT ${RET_VAL_libs} EQUAL 0)

		execute_process(COMMAND ${FREETYPECONFIG_EXECUTABLE} --cflags
			RESULT_VARIABLE RET_VAL_cflags
			OUTPUT_VARIABLE _freetype_pkgconfig_output
			OUTPUT_STRIP_TRAILING_WHITESPACE
			)
		if    (${RET_VAL_cflags} EQUAL 0 AND _freetype_pkgconfig_output)
			# freetype-config can print out more than one -I, so we need to chop it up
			# into a list and process each entry separately
			separate_arguments(_freetype_pkgconfig_output)
			foreach(value ${_freetype_pkgconfig_output})
				string(REGEX REPLACE "-I(.+)" "\\1" value "${value}")
				set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR} ${value})
			endforeach(value)
		endif (${RET_VAL_cflags} EQUAL 0 AND _freetype_pkgconfig_output)
	else  (FREETYPECONFIG_EXECUTABLE AND NOT MINGW)
		if    (MINGW)
			find_path(FREETYPE_INCLUDE_DIR freetype/freetype.h)
		else  (MINGW)
			find_path(FREETYPE_INCLUDE_DIR freetype2/freetype/freetype.h)
		endif (MINGW)

		find_library(FREETYPE_LIBRARIES
			NAMES
				freetype
				freetype.dll
				freetype6
			PATH_SUFFIXES
				dll
				lib64
				lib
				libs64
				libs
				libs/Win32
				libs/Win64
			PATHS
				$ENV{LD_LIBRARY_PATH}
				$ENV{LIBRARY_PATH}
				/usr
				/usr/local
				/usr/bin
				"${MINGWLIBS}"
			NO_DEFAULT_PATH
			)

		if    (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
			if    (NOT MINGW)
				set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR}/freetype2)
			endif (NOT MINGW)
		endif (FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
	endif(FREETYPECONFIG_EXECUTABLE AND NOT MINGW)
endif (NOT FREETYPE_LIBRARIES OR NOT FREETYPE_INCLUDE_DIR)

set(FREETYPE_INCLUDE_DIRS "${FREETYPE_INCLUDE_DIR}")

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Freetype DEFAULT_MSG FREETYPE_LIBRARIES FREETYPE_INCLUDE_DIR)
mark_as_advanced(FREETYPE_LIBRARIES FREETYPE_INCLUDE_DIR FREETYPECONFIG_EXECUTABLE)

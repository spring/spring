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

if (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

  # in cache already
  set(FREETYPE_FOUND TRUE)

else (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

  find_program(FREETYPECONFIG_EXECUTABLE NAMES freetype-config PATHS
     /opt/local/bin
  )

  #reset vars
  set(FREETYPE_LIBRARIES)
  set(FREETYPE_INCLUDE_DIR)

  # if freetype-config has been found
  if(FREETYPECONFIG_EXECUTABLE AND NOT MINGW)

    exec_program(${FREETYPECONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE FREETYPE_LIBRARIES)

    exec_program(${FREETYPECONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _freetype_pkgconfig_output)
    if(FREETYPE_LIBRARIES AND _freetype_pkgconfig_output)
      set(FREETYPE_FOUND TRUE)

      # freetype-config can print out more than one -I, so we need to chop it up
      # into a list and process each entry separately
      separate_arguments(_freetype_pkgconfig_output)
      foreach(value ${_freetype_pkgconfig_output})
        string(REGEX REPLACE "-I(.+)" "\\1" value "${value}")
        set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR} ${value})
      endforeach(value)
    endif(FREETYPE_LIBRARIES AND _freetype_pkgconfig_output)

    set( FREETYPE_LIBRARIES ${FREETYPE_LIBRARIES} CACHE STRING "The libraries for freetype" )

    mark_as_advanced(FREETYPE_LIBRARIES FREETYPE_INCLUDE_DIR)

  else(FREETYPECONFIG_EXECUTABLE AND NOT MINGW)
    if(MINGW)
      find_path (FREETYPE_INCLUDE_DIR freetype/freetype.h)
    else(MINGW)
      find_path (FREETYPE_INCLUDE_DIR freetype2/freetype/freetype.h)
      set (FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR}/freetype2)
    endif(MINGW)
    find_library(FREETYPE_LIBRARIES freetype)
    if(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
        set(FREETYPE_FOUND TRUE)
    endif(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
  endif(FREETYPECONFIG_EXECUTABLE AND NOT MINGW)


  if (FREETYPE_FOUND)
    if (NOT Freetype_FIND_QUIETLY)
       message(STATUS "Found Freetype: ${FREETYPE_LIBRARIES}")
    endif (NOT Freetype_FIND_QUIETLY)
  else (FREETYPE_FOUND)
    if (Freetype_FIND_REQUIRED)
       message(FATAL_ERROR "Could not find FreeType library")
    endif (Freetype_FIND_REQUIRED)
  endif (FREETYPE_FOUND)

endif (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

set(FREETYPE_INCLUDE_DIRS "${FREETYPE_INCLUDE_DIR}") 
mark_as_advanced(FREETYPE_LIBRARIES FREETYPECONFIG_EXECUTABLE)
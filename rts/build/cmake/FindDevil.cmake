# Downloaded from: http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/cmake/
# License: GPL v2, http://www-id.imag.fr/FLOWVR/manual/flowvr-suite-src/flowvr-render/COPYING
#
# Modifications:
# 2008.01.16 Tobi Vollebregt -- changed Devil->DEVIL for consistency
#                            -- added devil, ilu, ilut alternative names for MinGW
#                            -- removed "looking for devil" status message

# - Find devil
# Find the native DevIL includes and library
#
#  DEVIL_INCLUDE_DIR - where to find IL/il.h, etc.
#  DEVIL_LIBRARIES   - List of libraries when using IL.
#  DEVIL_FOUND       - True if IL found.

IF (DEVIL_INCLUDE_DIR AND DEVIL_IL_LIBRARY)
  # Already in cache, be silent
  SET(Devil_FIND_QUIETLY TRUE)
ENDIF (DEVIL_INCLUDE_DIR AND DEVIL_IL_LIBRARY)

FIND_LIBRARY(DEVIL_IL_LIBRARY
  NAMES IL devil DevIL
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr
  /usr/local
  /usr/bin
)

FIND_LIBRARY(DEVIL_ILU_LIBRARY
  NAMES ilu ILU
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr
  /usr/local
)

FIND_LIBRARY(DEVIL_ILUT_LIBRARY
  NAMES ilut ILUT
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr
  /usr/local
)

GET_FILENAME_COMPONENT(DEVIL_LIBRARY_DIR "${DEVIL_IL_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(DEVIL_LIBRARY_SUPER_DIR "${DEVIL_LIBRARY_DIR}" PATH)

FIND_PATH(DEVIL_INCLUDE_DIR IL/il.h
  PATHS
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  ${DEVIL_LIBRARY_SUPER_DIR}/include
  ENV CPATH
  /usr/local/include
  /usr/include
  NO_DEFAULT_PATH
)

IF (DEVIL_INCLUDE_DIR AND DEVIL_IL_LIBRARY)
   SET(DEVIL_FOUND TRUE)
   SET(DEVIL_LIBRARIES ${DEVIL_IL_LIBRARY} ${DEVIL_ILU_LIBRARY} ${DEVIL_ILUT_LIBRARY})
ELSE (DEVIL_INCLUDE_DIR AND DEVIL_IL_LIBRARY)
   SET(DEVIL_FOUND FALSE)
   SET(DEVIL_LIBRARIES)
ENDIF (DEVIL_INCLUDE_DIR AND DEVIL_IL_LIBRARY)

IF (DEVIL_FOUND)
   IF (NOT Devil_FIND_QUIETLY)
      MESSAGE(STATUS "Found DEVIL: ${DEVIL_LIBRARIES}")
   ENDIF (NOT Devil_FIND_QUIETLY)
ELSE (DEVIL_FOUND)
   IF (Devil_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could NOT find DEVIL library")
   ENDIF (Devil_FIND_REQUIRED)
ENDIF (DEVIL_FOUND)

MARK_AS_ADVANCED(
  DEVIL_LIBRARIES
  DEVIL_INCLUDE_DIR
  DEVIL_IL_LIBRARY
  DEVIL_ILU_LIBRARY
  DEVIL_ILUT_LIBRARY
  ) 

# Locate OpenAL
# This module defines
# OPENAL_LIBRARY
# OPENAL_FOUND, if false, do not try to link to OpenAL 
# OPENAL_INCLUDE_DIR, where to find the headers
#
# $OPENALDIR is an environment variable that would
# correspond to the ./configure --prefix=$OPENALDIR
# used in building OpenAL.
#
# Created by Eric Wing. This was influenced by the FindSDL.cmake module.

# This makes the presumption that you are include al.h like
# #include "al.h"
# and not 
# #include <AL/al.h>
# The reason for this is that the latter is not entirely portable.
# Windows/Creative Labs does not by default put their headers in AL/ and 
# OS X uses the convention <OpenAL/al.h>.
# 
# For Windows, Creative Labs seems to have added a registry key for their 
# OpenAL 1.1 installer. I have added that key to the list of search paths,
# however, the key looks like it could be a little fragile depending on 
# if they decide to change the 1.00.0000 number for bug fix releases.
# Also, they seem to have laid down groundwork for multiple library platforms
# which puts the library in an extra subdirectory. Currently there is only
# Win32 and I have hardcoded that here. This may need to be adjusted as 
# platforms are introduced.
# The OpenAL 1.0 installer doesn't seem to have a useful key I can use.
# I do not know if the Nvidia OpenAL SDK has a registry key.
# 
# For OS X, remember that OpenAL was added by Apple in 10.4 (Tiger). 
# To support the framework, I originally wrote special framework detection 
# code in this module which I have now removed with CMake's introduction
# of native support for frameworks.
# In addition, OpenAL is open source, and it is possible to compile on Panther. 
# Furthermore, due to bugs in the initial OpenAL release, and the 
# transition to OpenAL 1.1, it is common to need to override the built-in
# framework. 
# Per my request, CMake should search for frameworks first in
# the following order:
# ~/Library/Frameworks/OpenAL.framework/Headers
# /Library/Frameworks/OpenAL.framework/Headers
# /System/Library/Frameworks/OpenAL.framework/Headers
#
# On OS X, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# OPENAL_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

if (OPENAL_INCLUDE_DIR AND OPENAL_LIBRARY)
	set (OpenAL_FIND_QUIETLY TRUE)
endif (OPENAL_INCLUDE_DIR AND OPENAL_LIBRARY)

FIND_PATH(OPENAL_INCLUDE_DIR al.h
  HINTS
  $ENV{OPENALDIR}
  PATH_SUFFIXES include/AL include/OpenAL include AL
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
)

FIND_LIBRARY(OPENAL_LIBRARY_RAW 
  NAMES OpenAL al openal OpenAL32 soft_oal
  HINTS
  $ENV{OPENALDIR}
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /dll
  /sw
  /opt/local
  /opt/csw
  /opt
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
)

IF(OPENAL_LIBRARY_RAW)
#	string(REGEX REPLACE "\\.([Dd][Ll][Ll])" "" OPENAL_LIBRARY ${OPENAL_LIBRARY_RAW})
	SET(OPENAL_LIBRARY ${OPENAL_LIBRARY_RAW})
ENDIF(OPENAL_LIBRARY_RAW)

IF(OPENAL_LIBRARY AND OPENAL_INCLUDE_DIR)
  SET(OPENAL_FOUND TRUE)
ELSE(OPENAL_LIBRARY AND OPENAL_INCLUDE_DIR)
  SET(OPENAL_FOUND FALSE)
ENDIF(OPENAL_LIBRARY AND OPENAL_INCLUDE_DIR)

IF(OPENAL_FOUND)
  if (NOT OpenAL_FIND_QUIETLY)
    message(STATUS "Found OpenAL: ${OPENAL_LIBRARY}")
  endif (NOT OpenAL_FIND_QUIETLY)
ELSE(OPENAL_FOUND)
   if (OpenAL_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find OpenAL libraries")
   endif (OpenAL_FIND_REQUIRED)
   if (NOT OpenAL_FIND_QUITELY)
      message(STATUS "Could NOT find OpenAL libraries")
   endif (NOT OpenAL_FIND_QUITELY)
ENDIF(OPENAL_FOUND)


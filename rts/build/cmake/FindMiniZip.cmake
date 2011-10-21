# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find the MiniZip library (sub-part of zlib)
# Find the native MiniZip includes and library (static or shared)
#
#  MINIZIP_INCLUDE_DIR - where to find zip.h, unzip.h, ioapi.h, etc.
#  MINIZIP_LIBRARIES   - List of libraries when using minizip.
#  MINIZIP_FOUND       - True if minizip was found.

Include(FindPackageHandleStandardArgs)

If     (MINIZIP_INCLUDE_DIR)
  # Already in cache, be silent
  Set(MINIZIP_FIND_QUIETLY TRUE)
EndIf  (MINIZIP_INCLUDE_DIR)

Find_Path(MINIZIP_INCLUDE_DIR minizip/zip.h)

Set(MINIZIP_NAMES minizip)
Find_Library(MINIZIP_LIBRARY NAMES ${MINIZIP_NAMES})

# handle the QUIETLY and REQUIRED arguments and set MINIZIP_FOUND to TRUE if
# all listed variables are TRUE
Find_Package_Handle_Standard_Args(MINIZIP DEFAULT_MSG MINIZIP_LIBRARY MINIZIP_INCLUDE_DIR)

If     (MINIZIP_FOUND)
  Set(MINIZIP_LIBRARIES ${MINIZIP_LIBRARY})
Else   (MINIZIP_FOUND)
  Set(MINIZIP_LIBRARIES)
EndIf  (MINIZIP_FOUND)

Mark_As_Advanced(MINIZIP_LIBRARY MINIZIP_INCLUDE_DIR)

# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find the FontConfig library
# Find the FontConfig includes and library
#
#  FONTCONFIG_INCLUDE_DIR - where to find fontconfig.h etc.
#  FONTCONFIG_LIBRARIES   - List of libraries when using fontconfig.
#  FONTCONFIG_FOUND       - True if fontconfig was found.

Include(FindPackageHandleStandardArgs)

If     (FONTCONFIG_INCLUDE_DIR)
	# Already in cache, be silent
	Set(FONTCONFIG_FIND_QUIETLY TRUE)
EndIf  (FONTCONFIG_INCLUDE_DIR)

Find_Path(FONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h)

Set(FONTCONFIG_NAMES fontconfig fontconfig-1)
Find_Library(FONTCONFIG_LIBRARY NAMES ${FONTCONFIG_NAMES})

# handle the QUIETLY and REQUIRED arguments and set FONTCONFIG_FOUND to TRUE if
# all listed variables are TRUE
Find_Package_Handle_Standard_Args(FONTCONFIG DEFAULT_MSG FONTCONFIG_LIBRARY FONTCONFIG_INCLUDE_DIR)

If     (FONTCONFIG_FOUND)
	Set(FONTCONFIG_LIBRARIES ${FONTCONFIG_LIBRARY})
Else   (FONTCONFIG_FOUND)
	Set(FONTCONFIG_LIBRARIES)
EndIf  (FONTCONFIG_FOUND)

Mark_As_Advanced(FONTCONFIG_LIBRARY FONTCONFIG_INCLUDE_DIR)

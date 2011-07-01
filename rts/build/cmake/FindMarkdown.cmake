# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find Markdown
# Find the native _markdown_ binary, which may be used to convert _*.markdown_
# files to _*.html_.
# http://daringfireball.net/projects/markdown/
#
# See Pandoc (FindPandoc.cmake), for a more powerful utility.
#
#  MARKDOWN_BIN   - will be set to the Markdown executable (eg. markdown.exe)
#  MARKDOWN_FOUND - TRUE if Markdown was found

Include(FindPackageHandleStandardArgs)

If    (MARKDOWN_BIN)
	# Already in cache, be silent
	Set(Markdown_FIND_QUIETLY TRUE)
EndIf (MARKDOWN_BIN)

find_program(MARKDOWN_BIN
		NAMES markdown
		HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
		PATH_SUFFIXES bin
		DOC "Markdown executable"
	)

# handle the QUIETLY and REQUIRED arguments and set MARKDOWN_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Markdown DEFAULT_MSG MARKDOWN_BIN)

MARK_AS_ADVANCED(MARKDOWN_BIN)

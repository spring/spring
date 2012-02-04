# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find Markdown
# Find the native _markdown_ binary, which may be used to convert _*.markdown_
# files to _*.html_.
# http://daringfireball.net/projects/markdown/
#
# See Pandoc (FindPandoc.cmake), for a more powerful utility.
#
#  MARKDOWN_BIN      - will be set to the Markdown executable (eg. markdown.exe)
#  MARKDOWN_FOUND    - TRUE if Markdown was found
#  Markdown_MdToHtml - creates a string that may be executed on the cmd-line
#                      for converting a markdown file to HTML

Include(FindPackageHandleStandardArgs)

If    (MARKDOWN_BIN)
	# Already in cache, be silent
	Set(Markdown_FIND_QUIETLY TRUE)
EndIf (MARKDOWN_BIN)

find_program(MARKDOWN_BIN
		NAMES markdown markdown_py
		HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
		PATH_SUFFIXES bin
		DOC "Markdown executable"
	)

# handle the QUIETLY and REQUIRED arguments and set MARKDOWN_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Markdown DEFAULT_MSG MARKDOWN_BIN)

MARK_AS_ADVANCED(MARKDOWN_BIN)

If    (MARKDOWN_FOUND)
	Macro    (Markdown_MdToHtml var_command fileSrc fileDst)
		# There are at least two compleetly different versions of markdown.
		# The one on gentoo suports a lot of cmd-line switches,
		# the one on Ubuntu not. This should work with both:
		Set("${var_command}"
				${MARKDOWN_BIN} "${fileSrc}" > "${fileDst}")
	EndMacro (Markdown_MdToHtml)
EndIf (MARKDOWN_FOUND)

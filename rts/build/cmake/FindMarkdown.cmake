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
#  markdown_md_to_html - creates a string that may be executed on the cmd-line
#                      for converting a markdown file to HTML

include(FindPackageHandleStandardArgs)

if    (MARKDOWN_BIN)
	# Already in cache, be silent
	set(Markdown_FIND_QUIETLY TRUE)
endif (MARKDOWN_BIN)

find_program(MARKDOWN_BIN
		NAMES markdown markdown_py
		HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
		PATH_SUFFIXES bin
		DOC "Markdown executable"
	)

# handle the QUIETLY and REQUIRED arguments and set MARKDOWN_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(Markdown DEFAULT_MSG MARKDOWN_BIN)

mark_as_advanced(MARKDOWN_BIN)

if    (MARKDOWN_FOUND)
	macro    (markdown_md_to_html var_command fileSrc fileDst)
		# There are at least two compleetly different versions of markdown.
		# The one on gentoo suports a lot of cmd-line switches,
		# the one on Ubuntu not. This should work with both:
		set("${var_command}"
				${MARKDOWN_BIN} "${fileSrc}" > "${fileDst}")
	endmacro (markdown_md_to_html)
endif (MARKDOWN_FOUND)

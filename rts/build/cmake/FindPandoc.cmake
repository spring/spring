# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find Pandoc
# Find the native Pandoc binary
# If you need to convert files from one markup format into another,
# pandoc is your swiss-army knife.
# It can read markdown and (subsets of) reStructuredText, HTML, and LaTeX,
# and it can write markdown, reStructuredText, HTML, LaTeX, ConTeXt,
# Docbook XML, OpenDocument XML, GNU Texinfo, RTF, ODT, MediaWiki markup,
# groff man pages, and S5 HTML slide shows.
# http://johnmacfarlane.net/pandoc/
#
# See Markdown (FindMarkdown.cmake), for a more lightweight utility.
#
#  PANDOC_BIN      - will be set to the Pandoc executable (eg. pandoc.exe)
#  PANDOC_FOUND    - TRUE if Pandoc was found
#  pandoc_md_to_html - creates a string that may be executed on the cmd-line
#                    for converting a markdown file to HTML

include(FindPackageHandleStandardArgs)

if    (PANDOC_BIN)
	# Already in cache, be silent
	set(Pandoc_FIND_QUIETLY TRUE)
endif (PANDOC_BIN)

find_program(PANDOC_BIN
		NAMES pandoc
		HINTS "${MINGWDIR}" "${CMAKE_SOURCE_DIR}/installer"
		PATH_SUFFIXES bin
		DOC "Pandoc executable"
	)

# handle the QUIETLY and REQUIRED arguments and set PANDOC_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(Pandoc DEFAULT_MSG PANDOC_BIN)

mark_as_advanced(PANDOC_BIN)

if    (PANDOC_FOUND)
	macro    (pandoc_md_to_html var_command fileSrc fileDst title)
		set("${var_command}"
				"${PANDOC_BIN}"
				--from=markdown
				--to=html
				-s --variable="pagetitle:${title}"
				-o "${fileDst}"
				"${fileSrc}")
	endmacro (pandoc_md_to_html)
endif (PANDOC_FOUND)

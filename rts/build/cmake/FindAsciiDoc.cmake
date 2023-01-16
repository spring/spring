# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find AsciiDoc
# These tool is used to compile man pages
#
#  ASCIIDOC_BIN   - will be set to the AsciiDoc executable (eg. asciidoc)
#  ASCIIDOC_FOUND - TRUE if AsciiDoc was found

INCLUDE(FindPackageHandleStandardArgs)

# Already in cache, be silent
if (ASCIIDOC_BIN)
	SET(AsciiDoc_FIND_QUIETLY TRUE)
endif ()

find_program(ASCIIDOC_BIN
	NAMES asciidoc
	PATH_SUFFIXES bin
	DOC "AsciiDoc executable"
	)

# handle the QUIETLY and REQUIRED arguments and set ASCIIDOC_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AsciiDoc DEFAULT_MSG ASCIIDOC_BIN)

MARK_AS_ADVANCED(ASCIIDOC_BIN)

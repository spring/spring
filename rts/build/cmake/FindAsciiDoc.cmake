# - Find AsciiDoc
# Find the AsciiDoc binary, which is used to compile man pages
#
#  ASCIIDOC_BIN   - will be set to the AsciiDoc executable (eg. asciidoc)
#  ASCIIDOC_FOUND - TRUE if AsciiDoc was found

IF    (ASCIIDOC_BIN)
	# Already in cache, be silent
	SET(AsciiDoc_FIND_QUIETLY TRUE)
ENDIF (ASCIIDOC_BIN)

find_program(ASCIIDOC_BIN
	NAMES asciidoc
	PATH_SUFFIXES bin
	DOC "AsciiDoc executable"
	)

# handle the QUIETLY and REQUIRED arguments and set ASCIIDOC_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AsciiDoc DEFAULT_MSG ASCIIDOC_BIN)

MARK_AS_ADVANCED(ASCIIDOC_BIN)

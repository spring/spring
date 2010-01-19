# - Find AsciiDoc, XSLTProc and DocBook
# These toolds are used to compile man pages
#
#  ASCIIDOC_BIN   - will be set to the AsciiDoc executable (eg. asciidoc)
#  XSLTPROC_BIN   - will be set to the XSLTProc executable (eg. xsltproc)
#  DOCBOOK_XSL    - will be set to the DocBook XSL Style-Sheet (eg. /usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl)
#  ASCIIDOC_FOUND - TRUE if AsciiDoc was found
#  XSLTPROC_FOUND - TRUE if XSLTProc was found
#  DOCBOOK_FOUND  - TRUE if DocBook was found

# Already in cache, be silent
IF    (ASCIIDOC_BIN)
	SET(AsciiDoc_FIND_QUIETLY TRUE)
ENDIF (ASCIIDOC_BIN)
IF    (XSLTPROC_BIN)
	SET(XSLTProc_FIND_QUIETLY TRUE)
ENDIF (XSLTPROC_BIN)
IF    (DOCBOOK_XSL)
	SET(DocBook_FIND_QUIETLY TRUE)
ENDIF (DOCBOOK_XSL)

find_program(ASCIIDOC_BIN
	NAMES asciidoc
	PATH_SUFFIXES bin
	DOC "AsciiDoc executable"
	)

find_program(XSLTPROC_BIN
	NAMES xsltproc
	PATH_SUFFIXES bin
	DOC "XSLTProc executable"
	)

find_file(DOCBOOK_XSL
	NAMES docbook.xsl
	PATHS /usr /usr/share /usr/local /usr/local/share
	PATH_SUFFIXES
		xml/docbook/stylesheet/nwalsh/manpages
		sgml/docbook/xsl-stylesheets/manpages
	DOC "DocBook XSL Style-Sheet"
	)

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set ASCIIDOC_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AsciiDoc DEFAULT_MSG ASCIIDOC_BIN)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XSLTProc DEFAULT_MSG XSLTPROC_BIN)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DocBook  DEFAULT_MSG DOCBOOK_XSL)

MARK_AS_ADVANCED(ASCIIDOC_BIN XSLTPROC_BIN DOCBOOK_XSL)

# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find DocBook
# These tool is used to compile man pages
#
#  DOCBOOK_XSL    - will be set to the DocBook XSL Style-Sheet (eg. /usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl)
#  DOCBOOK_FOUND  - TRUE if DocBook was found

INCLUDE(FindPackageHandleStandardArgs)

# Already in cache, be silent
if (DOCBOOK_XSL)
	SET(DocBook_FIND_QUIETLY TRUE)
endif ()

find_file(DOCBOOK_XSL
	NAMES docbook.xsl
	PATHS /usr /usr/share /usr/local /usr/local/share
	PATH_SUFFIXES
		xml/docbook/stylesheet/nwalsh/manpages
		xml/docbook/stylesheet/nwalsh/1.78.1/manpages
		xml/docbook/stylesheet/nwalsh/1.79.0/manpages
		sgml/docbook/xsl-stylesheets/manpages
		xsl/docbook/manpages
	DOC "DocBook XSL Style-Sheet"
	)

if (NOT DOCBOOK_XSL)
	file(GLOB DOCBOOK_XSL / /usr/share/xml/docbook/xsl-stylesheets-*/manpages/docbook.xsl)
	if (DOCBOOK_XSL)
		list(GET DOCBOOK_XSL 0 DOCBOOK_XSL)
	endif ()
endif ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(DocBook  DEFAULT_MSG DOCBOOK_XSL)

MARK_AS_ADVANCED(DOCBOOK_XSL)

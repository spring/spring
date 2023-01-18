# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find XSLTProc
# These tool is used to compile man pages
#
#  XSLTPROC_BIN   - will be set to the XSLTProc executable (eg. xsltproc)
#  XSLTPROC_FOUND - TRUE if XSLTProc was found

INCLUDE(FindPackageHandleStandardArgs)

# Already in cache, be silent
if (XSLTPROC_BIN)
	SET(XSLTProc_FIND_QUIETLY TRUE)
endif ()

find_program(XSLTPROC_BIN
	NAMES xsltproc
	PATH_SUFFIXES bin
	DOC "XSLTProc executable"
	)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(XSLTProc DEFAULT_MSG XSLTPROC_BIN)

MARK_AS_ADVANCED(XSLTPROC_BIN)

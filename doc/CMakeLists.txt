
### make and install the man pages
if    (UNIX)
	FIND_PACKAGE(7zip)
	set(AsciiDoc_FIND_QUIETLY TRUE)
	FIND_PACKAGE(AsciiDoc)
	set(CREATE_MAN_PAGES)
	if    (7ZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
		execute_process(
			COMMAND "${ASCIIDOC_BIN}"
			RESULT_VARIABLE ASCIIDOC_RESULT
			ERROR_QUIET
			OUTPUT_QUIET
			ERROR_STRIP_TRAILING_WHITESPACE)
		if    (ASCIIDOC_RESULT EQUAL 0)
			set(CREATE_MAN_PAGES TRUE)
		else  (ASCIIDOC_RESULT EQUAL 0)
			set(CREATE_MAN_PAGES FALSE)
			message(STATUS "MAN pages will not be generated/installed (utility asciidoc is broken)")
		endif (ASCIIDOC_RESULT EQUAL 0)
	else  (7ZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
		set(MISSING_UTILS "")
		if    (NOT 7ZIP_FOUND)
			set(MISSING_UTILS "${MISSING_UTILS} 7z*")
		endif (NOT 7ZIP_FOUND)
		if    (NOT ASCIIDOC_FOUND)
			set(MISSING_UTILS "${MISSING_UTILS} asciidoc")
		endif (NOT ASCIIDOC_FOUND)
		if    (NOT XSLTPROC_FOUND)
			set(MISSING_UTILS "${MISSING_UTILS} xsltproc")
		endif (NOT XSLTPROC_FOUND)
		if    (NOT DOCBOOK_FOUND)
			set(MISSING_UTILS "${MISSING_UTILS} docbook-xsl")
		endif (NOT DOCBOOK_FOUND)
		message(STATUS "MAN pages will not be generated/installed; missing utilit(y|ies):${MISSING_UTILS}")
	endif (7ZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)

	if    (CREATE_MAN_PAGES)
		set(MAN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/manpages")
		set(MAN_BUILD_DIR  "${CMAKE_BINARY_DIR}/manpages")
		set(MAN_DEST_DIR   "${MANDIR}/man6")

		add_custom_target(manpages ALL
			COMMAND "${MAN_SOURCE_DIR}/make_manpages.sh"
				"${MAN_BUILD_DIR}"
				"${ASCIIDOC_BIN}"
				"${XSLTPROC_BIN}"
				"${DOCBOOK_XSL}"
				"${7ZIP_BIN}"
			WORKING_DIRECTORY "${MAN_SOURCE_DIR}")

		install(DIRECTORY "${MAN_BUILD_DIR}/" DESTINATION "${MAN_DEST_DIR}" FILES_MATCHING PATTERN REGEX ".*\\.6\\.gz$")
		message(STATUS "MAN pages will be installed to {INSTALL_PREFIX}/${MAN_DEST_DIR}")
	endif (CREATE_MAN_PAGES)
endif (UNIX)


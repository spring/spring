
### make and install the man pages
if    (UNIX)
	find_package(SevenZip)
	set(AsciiDoc_FIND_QUIETLY TRUE)
	find_package(AsciiDoc)
	set(CREATE_MAN_PAGES)
	if    (SEVENZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
		execute_process(
			COMMAND "${ASCIIDOC_BIN}"
			RESULT_VARIABLE ASCIIDOC_RESULT
			ERROR_QUIET
			OUTPUT_QUIET
			ERROR_STRIP_TRAILING_WHITESPACE)
		if    (ASCIIDOC_RESULT EQUAL 0)
			make_global_var(CREATE_MAN_PAGES TRUE)
		else  (ASCIIDOC_RESULT EQUAL 0)
			make_global_var(CREATE_MAN_PAGES FALSE)
			message(STATUS "MAN pages will not be generated/installed (utility asciidoc is broken)")
		endif (ASCIIDOC_RESULT EQUAL 0)
	else  (SEVENZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
		set(MISSING_UTILS "")
		if    (NOT SEVENZIP_FOUND)
			set(MISSING_UTILS "${MISSING_UTILS} 7z*")
		endif (NOT SEVENZIP_FOUND)
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
	endif (SEVENZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)

	if    (CREATE_MAN_PAGES)
		set(MAN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/manpages")
		set(MAN_BUILD_DIR  "${CMAKE_BINARY_DIR}/manpages")
		set(MAN_DEST_DIR   "${MANDIR}/man6")

		set(man_src
				"${MAN_SOURCE_DIR}/spring.6.txt")
		set(man_dst
				"${MAN_BUILD_DIR}/spring-dedicated.6.gz"
				"${MAN_BUILD_DIR}/spring-headless.6.gz"
				"${MAN_BUILD_DIR}/spring-legacy.6.gz"
				"${MAN_BUILD_DIR}/spring.6.gz")
		set_source_files_properties(${man_dst} PROPERTIES GENERATED TRUE)

		add_custom_command(
			OUTPUT ${man_dst}
			COMMAND "${MAN_SOURCE_DIR}/make_manpages.sh"
				"${MAN_BUILD_DIR}"
				"${ASCIIDOC_BIN}"
				"${XSLTPROC_BIN}"
				"${DOCBOOK_XSL}"
				"${SEVENZIP_BIN}"
			DEPENDS ${man_src}
			WORKING_DIRECTORY "${MAN_SOURCE_DIR}"
			COMMENT "manpages: Generating" VERBATIM
		)

		add_custom_target(manpages ALL DEPENDS ${man_dst})

		install(FILES ${man_dst} DESTINATION "${MAN_DEST_DIR}")

		message(STATUS "MAN pages will be installed to {INSTALL_PREFIX}/${MAN_DEST_DIR}")
	endif (CREATE_MAN_PAGES)
endif (UNIX)

option(USERDOCS_PLAIN "Install plain/text engine user-docs (README, LICENSE, etc.)" FALSE)
option(USERDOCS_HTML "Install HTML engine user-docs (README.html, LICENSE.html, etc.)" FALSE)

### make and install the user-doc pages (README, LICENSE, ...)
if     (WIN32)
	set(DOC_DEST_DIR "${DOCDIR}")
else   (WIN32)
	set(DOC_DEST_DIR "${DOCDIR}/engine")
endif  (WIN32)

set(userdoc_INSTALL_HTML FALSE)
if     (USERDOCS_HTML)
	set(Pandoc_FIND_QUIETLY TRUE)
	find_package(Pandoc)
	if     (NOT PANDOC_FOUND)
		set(Markdown_FIND_QUIETLY TRUE)
		find_package(Markdown)
	endif  (NOT PANDOC_FOUND)

	if     (PANDOC_FOUND OR MARKDOWN_FOUND)
		set(userdoc_INSTALL_HTML TRUE)

		# Generates "${name}.html" from "${name}(.markdown)"
		macro    (userdoc_generate_html name var_srcFile)
			set(DST_FILE_${name} "${CMAKE_CURRENT_BINARY_DIR}/${name}.html")

			if     (PANDOC_FOUND)
				pandoc_md_to_html(  convert_command_${name}
						"${${var_srcFile}}" "${DST_FILE_${name}}"
						"Spring RTS Game Engine - ${name}")
			else   ()
				markdown_md_to_html(convert_command_${name}
						"${${var_srcFile}}" "${DST_FILE_${name}}")
			endif  ()
			add_custom_command(
				OUTPUT "${DST_FILE_${name}}"
				COMMAND ${convert_command_${name}}
				DEPENDS "${${var_srcFile}}"
				COMMENT "Creating userdoc ${name}.html" VERBATIM
			)

			list(APPEND userdoc_html_generated "${DST_FILE_${name}}")

			set_source_files_properties("${DST_FILE_${name}}" PROPERTIES GENERATED TRUE)
		endmacro (userdoc_generate_html)
	else   (PANDOC_FOUND OR MARKDOWN_FOUND)
		message(WARNING "user-docs generation (html/text) is not possible (disable with USERDOCS_HTML=FALSE); missing utility: pandoc or markdown")
	endif  (PANDOC_FOUND OR MARKDOWN_FOUND)

	set(userdoc_html_generated "")
endif  (USERDOCS_HTML)

if     (USERDOCS_PLAIN OR userdoc_INSTALL_HTML)
	set(userdoc_plain "")

	macro    (userdoc_enlist name)
		set(userdoc_file_plain_${name} "${CMAKE_SOURCE_DIR}/${name}.markdown")
		if     (NOT EXISTS "${userdoc_file_plain_${name}}")
			set(userdoc_file_plain_${name} "${CMAKE_SOURCE_DIR}/${name}")
		endif  ()

		list(APPEND userdoc_plain "${userdoc_file_plain_${name}}")

		if     (userdoc_INSTALL_HTML)
			userdoc_generate_html("${name}" userdoc_file_plain_${name})
		endif  (userdoc_INSTALL_HTML)
	endmacro (userdoc_enlist)

	userdoc_enlist("AUTHORS")
	userdoc_enlist("README")
	userdoc_enlist("COPYING")
	userdoc_enlist("FAQ")
	userdoc_enlist("LICENSE")
	userdoc_enlist("THANKS")
endif  (USERDOCS_PLAIN OR userdoc_INSTALL_HTML)

set(userdoc_INSTALL_PLAIN ${USERDOCS_PLAIN})
if     (userdoc_INSTALL_PLAIN)
	set(userdoc_plain
			"${userdoc_plain}"
			"${CMAKE_SOURCE_DIR}/gpl-2.0.txt"
			"${CMAKE_SOURCE_DIR}/gpl-3.0.txt")

	install(FILES ${userdoc_plain} DESTINATION "${DOC_DEST_DIR}")

	message(STATUS "user-docs (plain/text) will be installed to {INSTALL_PREFIX}/${DOC_DEST_DIR}")
endif  (userdoc_INSTALL_PLAIN)

if     (userdoc_INSTALL_HTML)
	add_custom_target(userdocs ALL DEPENDS ${userdoc_html_generated})
	add_dependencies(generateSources userdocs)

	set(userdoc_html
			"${userdoc_html_generated}"
			"${CMAKE_SOURCE_DIR}/gpl-2.0.html"
			"${CMAKE_SOURCE_DIR}/gpl-3.0.html")

	install(FILES ${userdoc_html} DESTINATION "${DOC_DEST_DIR}")

	message(STATUS "user-docs (html/text) will be installed to {INSTALL_PREFIX}/${DOC_DEST_DIR}")
endif  (userdoc_INSTALL_HTML)


### make and install the man pages
if    (UNIX)
	FIND_PACKAGE(SevenZip)
	set(AsciiDoc_FIND_QUIETLY TRUE)
	FIND_PACKAGE(AsciiDoc)
	set(CREATE_MAN_PAGES)
	if    (SEVENZIP_FOUND AND ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
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

		Set(man_src
				"${MAN_SOURCE_DIR}/spring.6.txt")
		Set(man_dst
				"${MAN_BUILD_DIR}/spring-dedicated.6.gz"
				"${MAN_BUILD_DIR}/spring-headless.6.gz"
				"${MAN_BUILD_DIR}/spring-legacy.6.gz"
				"${MAN_BUILD_DIR}/spring.6.gz")
		set_source_files_properties(${man_dst} PROPERTIES GENERATED TRUE)

		Add_Custom_Command(
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


### make and install the user-doc pages (README, LICENSE, ...)
If     (WIN32)
	# We do not need the plain text version of the docu under windows
	Set(USERDOCS_PLAIN FALSE CACHE BOOL "Install plain/text engine user-docs (README, LICENSE, etc.)")
	Set(USERDOCS_HTML  TRUE  CACHE BOOL "Install HTML engine user-docs (README.html, LICENSE.html, etc.)")
	Set(DOC_DEST_DIR "${DOCDIR}")
Else   (WIN32)
	# TODO re-enable PLAIN on UNIX, once the correct version is set in ../CMakeLists.txt and make it a CACHE var then, also
	Set(USERDOCS_PLAIN FALSE       BOOL "Install plain/text engine user-docs (README, LICENSE, etc.)")
	Set(USERDOCS_HTML  FALSE CACHE BOOL "Install HTML engine user-docs (README.html, LICENSE.html, etc.)")
	Set(DOC_DEST_DIR "${DOCDIR}/engine")
EndIf  (WIN32)

Set(userdoc_INSTALL_HTML FALSE)
If     (USERDOCS_HTML)
	set(Pandoc_FIND_QUIETLY TRUE)
	FIND_PACKAGE(Pandoc)
	If     (NOT PANDOC_FOUND)
		Set(Markdown_FIND_QUIETLY TRUE)
		Find_Package(Markdown)
	EndIf  (NOT PANDOC_FOUND)

	If     (PANDOC_FOUND OR MARKDOWN_FOUND)
		Set(userdoc_INSTALL_HTML TRUE)

		# Generates "${name}.html" from "${name}(.markdown)"
		Macro    (Userdoc_GenerateHtml name var_srcFile)
			Set(DST_FILE_${name} "${CMAKE_CURRENT_BINARY_DIR}/${name}.html")

			If     (PANDOC_FOUND)
				Pandoc_MdToHtml(  convert_command_${name}
						"${${var_srcFile}}" "${DST_FILE_${name}}"
						"Spring RTS Game Engine - ${name}")
			Else   ()
				Markdown_MdToHtml(convert_command_${name}
						"${${var_srcFile}}" "${DST_FILE_${name}}")
			EndIf  ()
			add_custom_command(
				OUTPUT "${DST_FILE_${name}}"
				COMMAND ${convert_command_${name}}
				DEPENDS "${${var_srcFile}}"
				COMMENT "Creating userdoc ${name}.html" VERBATIM
			)

			List(APPEND userdoc_html_generated "${DST_FILE_${name}}")

			set_source_files_properties("${DST_FILE_${name}}" PROPERTIES GENERATED TRUE)
		EndMacro (Userdoc_GenerateHtml)
	Else   (PANDOC_FOUND OR MARKDOWN_FOUND)
		Message(WARNING "user-docs generation (html/text) is not possible (disable with USERDOCS_HTML=FALSE); missing utility: pandoc or markdown")
	EndIf  (PANDOC_FOUND OR MARKDOWN_FOUND)

	Set(userdoc_html_generated "")
EndIf  (USERDOCS_HTML)

If     (USERDOCS_PLAIN OR userdoc_INSTALL_HTML)
	Set(userdoc_plain "")

	Macro    (Userdoc_Enlist name)
		Set(userdoc_file_plain_${name} "${CMAKE_SOURCE_DIR}/${name}.markdown")
		If     (NOT EXISTS "${userdoc_file_plain_${name}}")
			Set(userdoc_file_plain_${name} "${CMAKE_SOURCE_DIR}/${name}")
		EndIf  ()

		List(APPEND userdoc_plain "${userdoc_file_plain_${name}}")

		If     (userdoc_INSTALL_HTML)
			Userdoc_GenerateHtml("${name}" userdoc_file_plain_${name})
		EndIf  (userdoc_INSTALL_HTML)
	EndMacro (Userdoc_Enlist)

	Userdoc_Enlist("AUTHORS")
	Userdoc_Enlist("README")
	Userdoc_Enlist("COPYING")
	Userdoc_Enlist("FAQ")
	Userdoc_Enlist("LICENSE")
	Userdoc_Enlist("THANKS")
EndIf  (USERDOCS_PLAIN OR userdoc_INSTALL_HTML)

Set(userdoc_INSTALL_PLAIN ${USERDOCS_PLAIN})
If     (userdoc_INSTALL_PLAIN)
	Set(userdoc_plain
			"${userdoc_plain}"
			"${CMAKE_SOURCE_DIR}/gpl-2.0.txt"
			"${CMAKE_SOURCE_DIR}/gpl-3.0.txt")

	Install(FILES ${userdoc_plain} DESTINATION "${DOC_DEST_DIR}")

	Message(STATUS "user-docs (plain/text) will be installed to {INSTALL_PREFIX}/${DOC_DEST_DIR}")
EndIf  (userdoc_INSTALL_PLAIN)

If     (userdoc_INSTALL_HTML)
	add_custom_target(userdocs ALL DEPENDS ${userdoc_html_generated})
	Add_Dependencies(generateSources userdocs)

	Set(userdoc_html
			"${userdoc_html_generated}"
			"${CMAKE_SOURCE_DIR}/gpl-2.0.html"
			"${CMAKE_SOURCE_DIR}/gpl-3.0.html")

	Install(FILES ${userdoc_html} DESTINATION "${DOC_DEST_DIR}")

	Message(STATUS "user-docs (html/text) will be installed to {INSTALL_PREFIX}/${DOC_DEST_DIR}")
EndIf  (userdoc_INSTALL_HTML)



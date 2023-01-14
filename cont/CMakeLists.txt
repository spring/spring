# required for man pages (optional) and base files (mandatory)

### Make base-files (aka sdz's)
add_subdirectory(base)

### Install base-files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${DATADIR}
		PATTERN ".gitignore" EXCLUDE
		PATTERN "base" EXCLUDE
		PATTERN "freedesktop" EXCLUDE
		PATTERN "CMakeLists.txt" EXCLUDE
		PATTERN "CMakeCache" EXCLUDE
		PATTERN "CMakeFiles" EXCLUDE
		PATTERN "Makefile" EXCLUDE
		PATTERN "cmake_install.cmake" EXCLUDE
		PATTERN "springrc-template-insource.txt" EXCLUDE
	)

### Install freedesktop-files, mime-types etc.
if    (UNIX)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/applications/spring.desktop
		DESTINATION
			${APPLICATIONS_DIR}
		)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/mime/spring.xml
		DESTINATION
			${MIME_DIR}/packages
		)
	install (
		FILES
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/pixmaps/spring.png
			${CMAKE_CURRENT_SOURCE_DIR}/freedesktop/pixmaps/application-x-spring-demo.png
		DESTINATION
			${PIXMAPS_DIR}
		)
endif ()

if (NOT EXISTS ${Spring_BINARY_DIR}/springsettings.cfg)
	if(WIN32)
		set(PLACEHOLDER ";")
	else ()
		set(PLACEHOLDER ":")
	endif ()
	set("SpringDataDirInSource" "${Spring_BINARY_DIR}${PLACEHOLDER}${Spring_SOURCE_DIR}/cont")
	set("FontFile" "${Spring_SOURCE_DIR}/cont/fonts/FreeSansBold.otf")
	set("SmallFontFile" "${Spring_SOURCE_DIR}/cont/fonts/FreeSansBold.otf")
	configure_file(springrc-template-insource.txt ${Spring_BINARY_DIR}/springsettings.cfg)
else ()
	message(STATUS "${Spring_BINARY_DIR}/springsettings.cfg already exists!")
endif ()

### Install dependencies (windows, mingwlibs DLLs)
if    (MINGW)
	install (DIRECTORY ${MINGWLIBS}/dll/ DESTINATION ${BINDIR} PATTERN "*.dll")
endif ()


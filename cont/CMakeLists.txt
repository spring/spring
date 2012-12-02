# required for man pages (optional) and base files (mandatory)
FIND_PACKAGE(SevenZip REQUIRED)

### Make base-files (aka sdz's)
file (TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/base" BASE_BUILD_DIR)
if    (CMAKE_HOST_WIN32)
	# As TO_NATIVE_PATH fails on windows, we need to do it manually
	string(REPLACE "/" "\\" BASE_BUILD_DIR "${BASE_BUILD_DIR}")
	set(BASE_BUILD_COMMAND call make_gamedata_arch.bat)
else  (CMAKE_HOST_WIN32)
	set(BASE_BUILD_COMMAND sh ./make_gamedata_arch.sh)
endif (CMAKE_HOST_WIN32)
add_custom_target(gamedata ALL
	COMMAND ${BASE_BUILD_COMMAND}
		${BASE_BUILD_DIR}
		${SEVENZIP_BIN}
	WORKING_DIRECTORY
		"${CMAKE_CURRENT_SOURCE_DIR}/base"
	)

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
	)

### Install content
install (DIRECTORY ${CMAKE_BINARY_DIR}/base DESTINATION ${DATADIR})


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
endif (UNIX)


### Install dependencies (windows, mingwlibs DLLs)
if    (MINGW)
	install (DIRECTORY ${MINGWLIBS}/dll/ DESTINATION ${BINDIR} PATTERN "*.dll")
	if(OPENMP_FOUND)
		# note we need to use gcc to find the library cause it isn't at the default /usr/lib/location
		#execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-print-file-name=libgomp-1.dll" OUTPUT_VARIABLE GOMP_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)
		#install(FILES ${GOMP_LIBRARY} DESTINATION ${BINDIR})

		#execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-print-file-name=libstdc++-6.dll" OUTPUT_VARIABLE STDCXX_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)
		#install(FILES ${STDCXX_LIBRARY} DESTINATION ${BINDIR})

		# pthreadGC2 is a dependency of gomp
		find_library(PTHREADGC2_LIBRARY NAMES pthreadGC2)
		install(FILES ${PTHREADGC2_LIBRARY} DESTINATION ${BINDIR})
	endif(OPENMP_FOUND)
endif (MINGW)


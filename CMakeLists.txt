### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
project(Spring)

# This is needed, as CMake, or at least soem versions of it (eg. 2.8),
# falsely use the ".so" suffix unde Mac OS X for MODULE's
macro    (FixLibName targetName)
	IF    (UNIX)
		SET_TARGET_PROPERTIES(${targetName} PROPERTIES PREFIX "lib")
		IF    (APPLE)
			SET_TARGET_PROPERTIES(${targetName} PROPERTIES SUFFIX ".dylib")
		ENDIF (APPLE)
	ENDIF (UNIX)
endmacro (FixLibName targetName)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

### Install path (relative to CMAKE_INSTALL_PREFIX)
if    (UNIX AND NOT MINGW)
	set (BINDIR  "bin"                CACHE STRING "Where to install binaries")
	set (LIBDIR  "lib"                CACHE STRING "Where to install libraries")
	set (MANDIR  "share/man"          CACHE STRING "Where to install man pages")
	set (DATADIR "share/games/spring" CACHE STRING "Where to install game content")

	set (APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set (PIXMAPS_DIR "share/pixmaps" CACHE STRING "Where to install icons")
	set (MIME_DIR "share/mime"CACHE STRING "Where MIME definitions are located")
	set (STABS_DEBUG_SYMBOLS FALSE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
else  (UNIX AND NOT MINGW)
	set (BINDIR  "." CACHE STRING "Where to install binaries")
	set (LIBDIR  "." CACHE STRING "Where to install libraries")
	set (DATADIR "." CACHE STRING "Where to install game content")

	set(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs package")
	if    (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
		MESSAGE(FATAL_ERROR "MINGWLIBS is not a valid directory")
	endif (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
	include_directories(${MINGWLIBS}/include)
	set(CMAKE_LIBRARY_PATH ${MINGWLIBS}/dll)
	set(CMAKE_PREFIX_PATH ${MINGWLIBS})

	add_definitions(-D_WIN32_WINNT=0x500) ## win2k is minimum
	# some cross-compile fixes
	set (CMAKE_FIND_ROOT_PATH ${MINGWLIBS} ${CMAKE_FIND_ROOT_PATH})
	set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	set (STABS_DEBUG_SYMBOLS FALSE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
endif (UNIX AND NOT MINGW)

# In theory, it should be possible to set this to LIBDIR instead
# (next two are relative to CMAKE_INSTALL_PREFIX)
set (AI_LIBS_DIR "${DATADIR}"     CACHE STRING "Where to install Skirmish AI libraries")
set (AI_DATA_DIR "${AI_LIBS_DIR}" CACHE STRING "Where to install Skirmish AI additional files (eg. configuration)")
set (AI_TYPES    "ALL"            CACHE STRING "Which AI Interfaces (and Skirmish AIs using them) to build [ALL|NATIVE|JAVA|NONE]")

set(SYNCCHECK TRUE CACHE BOOL "Check sync during gameplay (Required for online play!)")
if    (SYNCCHECK)
	add_definitions(-DSYNCCHECK)
endif (SYNCCHECK)

if    (WIN32)
	set(NO_AVI FALSE CACHE BOOL "Disable in-game video recording")
	if    (NOT NO_AVI)
		add_definitions(-DAVI_CAPTURING)
	endif (NOT NO_AVI)
	set(WINDOWS_CONSOLE FALSE CACHE BOOL "Show a visible cmd.exe with stdout when spring runs")
	if    (NOT WINDOWS_CONSOLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
	endif (NOT WINDOWS_CONSOLE)
endif (WIN32)

if (CMAKE_HOST_WIN32)
	set(PATH_SEP_H      "\\")
	set(PATH_DELIM_H    ";")
	set(ABS_DIR_REGEX_H "^[a-zA-Z]:\\")
else (CMAKE_HOST_WIN32)
	set(PATH_SEP_H      "/")
	set(PATH_DELIM_H    ":")
	set(ABS_DIR_REGEX_H "^/")
endif(CMAKE_HOST_WIN32)
if (WIN32)
	set(PATH_SEP_T      "\\")
	set(PATH_DELIM_T    ";")
	set(ABS_DIR_REGEX_T "^[a-zA-Z]:\\")
else (WIN32)
	set(PATH_SEP_T      "/")
	set(PATH_DELIM_T    ":")
	set(ABS_DIR_REGEX_T "^/")
endif(WIN32)

set(BUILTIN_DATADIRS "")
if    (NOT MINGW)
	set (SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	set (SPRING_DATADIR_INTERNAL "")
	if    (SPRING_DATADIR)
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR}${PATH_DELIM_T}")
	endif (SPRING_DATADIR)
	if    (CMAKE_INSTALL_PREFIX)
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${DATADIR}")
	endif (CMAKE_INSTALL_PREFIX)
	set(BUILTIN_DATADIRS "${SPRING_DATADIR_INTERNAL}")
endif (NOT MINGW)
if    (NOT "${AI_LIBS_DIR}" STREQUAL "${DATADIR}")
	# If the AI library dir is not the same as the normal data dir,
	# add it to the internal list of data dirs
	set(BUILTIN_DATADIRS "${BUILTIN_DATADIRS}${PATH_DELIM_T}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${AI_LIBS_DIR}")
endif (NOT "${AI_LIBS_DIR}" STREQUAL "${DATADIR}")
if    (NOT "${AI_DATA_DIR}" STREQUAL "${DATADIR}" AND NOT "${AI_DATA_DIR}" STREQUAL "${AI_LIBS_DIR}")
	# If the AI data dir is not the same as the normal data dir
	# nor the AI library dir, add it to the internal list of data dirs
	set(BUILTIN_DATADIRS "${BUILTIN_DATADIRS}${PATH_DELIM_T}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${AI_DATA_DIR}")
endif (NOT "${AI_DATA_DIR}" STREQUAL "${DATADIR}" AND NOT "${AI_DATA_DIR}" STREQUAL "${AI_LIBS_DIR}")
add_definitions(-DSPRING_DATADIR="${BUILTIN_DATADIRS}")

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if    (CUSTOM_COMPILER_DEFINITIONS)
	add_definitions(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)

if    (MINGW)
	set(SDL_INCLUDE_DIR "${MINGWLIBS}/include/SDL")
	FIND_PACKAGE(Windres)
endif (MINGW)
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

FIND_PACKAGE(Boost 1.35.0 COMPONENTS thread regex program_options system signals REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(Devil REQUIRED)

### mingwlibs
if     (MINGW)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_MODULE_PREFIX "")
elseif (UNIX)
	# unix specifics
else   (UNIX)
	# mac
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	INCLUDE_DIRECTORIES("${COREFOUNDATION_LIBRARY}/Headers")
endif  (MINGW)

### Compiler flags and defines based on build type
INCLUDE(TestCXXFlags)
if    (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 64bit environment")
	set (MARCH_FLAG CACHE STRING "CPU optimization (use i686 for generic optimization)")
else  (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 32bit environment")
	set (MARCH_FLAG "i686" CACHE STRING "CPU optimization (use i686 for generic optimization)")
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

if    ($ENV{CXX} MATCHES "icpc")
	# intel C/C++ compiler fix; it does not support these flags:
	# "-march -mfpmath -msse -ggdb"
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -axK") # SSE1
	set(CXX_FLAGS_DEBUG_ADDITIONAL "-g -debug full")
else  ($ENV{CXX} MATCHES "icpc")
	if    (MARCH_FLAG)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${MARCH_FLAG}")
	endif (MARCH_FLAG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SSE_FLAGS}") # SSE1
	if    (MINGW)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads") # SSE1
	endif (MINGW)

	set(CXX_FLAGS_DEBUG_ADDITIONAL "")
endif ($ENV{CXX} MATCHES "icpc")

set(SIGNAL_NANS FALSE CACHE BOOL "Enable NaN-Signalling")
if (SIGNAL_NANS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsignaling-nans")
endif(SIGNAL_NANS)

# set the default build type
if    (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RELWITHDEBINFO CACHE STRING
		"Choose the type of build, options are: RELWITHDEBINFO DEBUG DEBUG2 DEBUG3 RELEASE PROFILE"
		FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# This would produce the maximum on warnings
#set(DEBUG_WARNINGS "-Wall -Wconversion")
# This should produce a sane ammount of warnings
set(DEBUG_WARNINGS "-Wall -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IEEE_FP_FLAG} -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
add_definitions(-DSTREFLOP_SSE) # would break AI compiling, but is undefined in ./AI/CMakeLists.txt
if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif (UNIX)
set(CMAKE_CXX_FLAGS_DEBUG           "${CXX_FLAGS_DEBUG_ADDITIONAL} -O1 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG2          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG3          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 ${DEBUG_WARNINGS} -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE         "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE         "-O2 -pg ${DEBUG_WARNINGS} -DNDEBUG")
set(CMAKE_MODULE_LINKER_FLAGS_PROFILE "-pg")
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg")
if (STABS_DEBUG_SYMBOLS)
	set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG}  -gstabs")
	set(CMAKE_CXX_FLAGS_DEBUG2          "${CMAKE_CXX_FLAGS_DEBUG2} -gstabs")
	set(CMAKE_CXX_FLAGS_DEBUG3          "${CMAKE_CXX_FLAGS_DEBUG3} -gstabs")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstabs")
else  (STABS_DEBUG_SYMBOLS)
	set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG}  -ggdb1")
	set(CMAKE_CXX_FLAGS_DEBUG2          "${CMAKE_CXX_FLAGS_DEBUG2} -ggdb2")
	set(CMAKE_CXX_FLAGS_DEBUG3          "${CMAKE_CXX_FLAGS_DEBUG3} -ggdb3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
endif (STABS_DEBUG_SYMBOLS)

if (NOT MINGW AND NOT APPLE)
	set (PIC_FLAG "-fpic")
elseif (APPLE)
	set (PIC_FLAG "-fPIC")
elseif (MINGW)
	set (CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-auto-import")
endif  (NOT MINGW AND NOT APPLE)

### Where the other CMakeLists.txt are
if    (SDL_FOUND)
	Add_Subdirectory(rts)
endif (SDL_FOUND)
if    (NOT "${AI_TYPES}" STREQUAL "NONE")
	Add_Subdirectory(AI)
endif (NOT "${AI_TYPES}" STREQUAL "NONE")
if    (SDL_FOUND)
	Add_Subdirectory(tools/DedicatedServer)
endif (SDL_FOUND)
if (WIN32)
	Add_Subdirectory(tools/ArchiveMover)
endif (WIN32)

Add_Subdirectory(tools/unitsync)

# required for man pages (optional) and base files (mandatory)
FIND_PACKAGE(7zip REQUIRED)

### make and install the man pages
if    (UNIX AND NOT MINGW)
	set(AsciiDoc_FIND_QUIETLY TRUE)
	FIND_PACKAGE(AsciiDoc)
	set(CREATE_MAN_PAGES)
	if    (ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
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
	else  (ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)
		set(MISSING_UTILS "")
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
	endif (ASCIIDOC_FOUND AND XSLTPROC_FOUND AND DOCBOOK_FOUND)

	if    (CREATE_MAN_PAGES)
		set(MAN_SOURCE_DIR "${CMAKE_SOURCE_DIR}/doc/manpages")
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
endif (UNIX AND NOT MINGW)

### make the basefiles (aka sdz's)
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
		${7ZIP_BIN}
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cont/base")

### install base data files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cont/ DESTINATION ${DATADIR} PATTERN ".gitignore" EXCLUDE PATTERN "base" EXCLUDE)
install (DIRECTORY ${CMAKE_BINARY_DIR}/base DESTINATION ${DATADIR})

### Install freedesktop-files, mime-types etc.
if    (UNIX AND NOT MINGW)
	install (FILES installer/freedesktop/applications/spring.desktop DESTINATION ${APPLICATIONS_DIR})
	install (FILES installer/freedesktop/mime/spring.xml DESTINATION ${MIME_DIR}/packages)
	install (FILES installer/freedesktop/pixmaps/spring.png installer/freedesktop/pixmaps/application-x-spring-demo.png DESTINATION ${PIXMAPS_DIR})
else  (UNIX AND NOT MINGW)
	install (DIRECTORY ${MINGWLIBS}/dll/ DESTINATION ${BINDIR} PATTERN "*.dll")
endif (UNIX AND NOT MINGW)

# additional tools
Add_Subdirectory(tools/DemoTool)

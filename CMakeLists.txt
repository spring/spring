### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
project(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

### Install path (relative to CMAKE_INSTALL_PREFIX)
if (UNIX AND NOT MINGW)
	set (BINDIR "bin" CACHE STRING "Where to install binaries")
	set (LIBDIR "lib" CACHE STRING "Where to install libraries")
	set (DATADIR "share/games/spring" CACHE STRING "Where to install game content")
	set (AIDIR "${LIBDIR}/spring" CACHE STRING "Where AIs are installed")

	set (APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set (PIXMAPS_DIR "share/pixmaps" CACHE STRING "Where to install icons")
	set (MIME_DIR "share/mime"CACHE STRING "Where MIME definitions are located")
	set(STABS_DEBUG_SYMBOLS FALSE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
else (UNIX AND NOT MINGW)
	set (BINDIR "." CACHE STRING "Where to install binaries")
	set (LIBDIR "." CACHE STRING "Where to install libraries")
	set (DATADIR "." CACHE STRING "Where to install game content")
	set (AIDIR "." CACHE STRING "Where AIs are installed")

	SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs package")
	if (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
		MESSAGE(FATAL_ERROR "MINGWLIBS is not a valid directory")
	endif (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
	include_directories(${MINGWLIBS}/include)
	set(CMAKE_LIBRARY_PATH ${MINGWLIBS}/dll)
	set(CMAKE_PREFIX_PATH ${MINGWLIBS})
	
	# some cross-compile fixes
	set (CMAKE_FIND_ROOT_PATH ${MINGWLIBS} ${CMAKE_FIND_ROOT_PATH})
	#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	set(STABS_DEBUG_SYMBOLS TRUE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
endif (UNIX AND NOT MINGW)

### Usr-definable build options
set (AIINTERFACES "ALL" CACHE STRING "Which AI Interfaces (and AIs using them) to build [ALL|NATIVE|JAVA|NONE]")

SET(SYNCCHECK TRUE CACHE BOOL "Check sync during gameplay (Required for online play!)")
if (SYNCCHECK)
	ADD_DEFINITIONS(-DSYNCCHECK)
endif (SYNCCHECK)

IF (WIN32)
	SET(NO_AVI FALSE CACHE BOOL "Disable in-game video recording")
	if (NO_AVI)
		ADD_DEFINITIONS(-DNO_AVI)
	endif (NO_AVI)
	SET(WINDOWS_CONSOLE FALSE CACHE BOOL "Show a visible cmd.exe with stdout when spring runs")
	IF (NOT WINDOWS_CONSOLE)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
	ENDIF (NOT WINDOWS_CONSOLE)
ELSE (WIN32)
	ADD_DEFINITIONS(-DNO_AVI)
ENDIF(WIN32)

IF (CMAKE_HOST_WIN32)
	SET(PATH_DELIM ";")
ELSE (CMAKE_HOST_WIN32)
	SET(PATH_DELIM ":")
ENDIF(CMAKE_HOST_WIN32)

if (NOT MINGW)
	set(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	set (SPRING_DATADIR_INTERNAL "")
	if (SPRING_DATADIR)
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR}${PATH_DELIM}")
	endif (SPRING_DATADIR)
	if (NOT "${DATADIR}" STREQUAL "${AIDIR}")
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}/${AIDIR}${PATH_DELIM}")
	endif (NOT "${DATADIR}" STREQUAL "${AIDIR}")
	set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}/${DATADIR}")
	ADD_DEFINITIONS(-DSPRING_DATADIR="${SPRING_DATADIR_INTERNAL}")
endif (NOT MINGW)

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if (CUSTOM_COMPILER_DEFINITIONS)
	ADD_DEFINITIONS(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)


if (MINGW)
	SET(SDL_INCLUDE_DIR "${MINGWLIBS}/include/SDL")
endif (MINGW)
FIND_PACKAGE(SDL)
if (SDL_FOUND)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
else (SDL_FOUND)
	message("SDL not found on your system, disabling:")
	message("* spring (engine)")
	message("* AIs")
endif (SDL_FOUND)

FIND_PACKAGE(Boost 1.35.0 COMPONENTS thread regex program_options system REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(Devil REQUIRED)

### mingwlibs
if (MINGW)
	SET(CMAKE_SHARED_LIBRARY_PREFIX "")
	SET(CMAKE_SHARED_MODULE_PREFIX "")
elseif (UNIX)
	# unix specifics
else (UNIX)
	# mac
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	INCLUDE_DIRECTORIES("${COREFOUNDATION_LIBRARY}/Headers")
endif (MINGW)

### Compiler flags and defines based on build type
INCLUDE(TestCXXAcceptsVisibilityFlag)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 64bit environment")
	set (MARCH_FLAG CACHE STRING "CPU optimization (use i686 for generic optimization)")
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 32bit environment")
	set (MARCH_FLAG "i686" CACHE STRING "CPU optimization (use i686 for generic optimization)")
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

IF ($ENV{CXX} MATCHES "icpc")
	# intel C/C++ compiler fix; it does not support these flags:
	# "-march -mfpmath -msse -ggdb"
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -axK") # SSE1
	SET(CXX_FLAGS_DEBUG_ADDITIONAL "-g -debug full")
ELSE ($ENV{CXX} MATCHES "icpc")
	IF (MARCH_FLAG)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${MARCH_FLAG}")
	ENDIF (MARCH_FLAG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -mfpmath=sse") # SSE1
	IF (MINGW)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads") # SSE1
	ENDIF (MINGW)

	SET(CXX_FLAGS_DEBUG_ADDITIONAL "")
ENDIF ($ENV{CXX} MATCHES "icpc")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math -fsignaling-nans")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee-fp -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
ADD_DEFINITIONS(-DSTREFLOP_SSE) # would break AI compiling, but is undefined in ./AI/CMakeLists.txt
IF (UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF (UNIX)
SET(CMAKE_CXX_FLAGS_DEBUG          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O1 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG2         "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG3         "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG -fomit-frame-pointer")
SET(CMAKE_CXX_FLAGS_PROFILE        "-O2 -pg -Wall -DNDEBUG")
IF (STABS_DEBUG_SYMBOLS)
	SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}  -gstabs")
	SET(CMAKE_CXX_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2} -gstabs")
	SET(CMAKE_CXX_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3} -gstabs")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstabs")
ELSE (STABS_DEBUG_SYMBOLS)
	SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}  -ggdb1")
	SET(CMAKE_CXX_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2} -ggdb2")
	SET(CMAKE_CXX_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3} -ggdb3")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
ENDIF (STABS_DEBUG_SYMBOLS)

if (NOT MINGW AND NOT APPLE)
	set (PIC_FLAG "-fpic")
elseif (APPLE)
	set (PIC_FLAG "-fPIC")
elseif (MINGW)
	set (CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-auto-import")
endif (NOT MINGW AND NOT APPLE)

### Where the other CMakeLists.txt are
if (SDL_FOUND)
	Add_Subdirectory(rts)
	Add_Subdirectory(AI)
endif (SDL_FOUND)
Add_Subdirectory(tools/DedicatedServer)
Add_Subdirectory(tools/unitsync)

### make the basefiles (aka sdz's)
if (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(CMAKE_HOST_WIN32)

### install base data files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".gitignore" EXCLUDE)
install (DIRECTORY ${CMAKE_BINARY_DIR}/base DESTINATION ${DATADIR})

### Install freedesktop-files, mime-types etc.
if (UNIX AND NOT MINGW)
	install (FILES installer/freedesktop/applications/spring.desktop DESTINATION ${APPLICATIONS_DIR})
	install (FILES installer/freedesktop/mime/spring.xml DESTINATION ${MIME_DIR}/packages)
	install (FILES installer/freedesktop/pixmaps/spring.png installer/freedesktop/pixmaps/application-x-spring-demo.png DESTINATION ${PIXMAPS_DIR})
else (UNIX AND NOT MINGW)
	install (DIRECTORY ${MINGWLIBS}/dll/ DESTINATION ${BINDIR} PATTERN "*.dll")
endif (UNIX AND NOT MINGW)

# additional tools

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)
	add_subdirectory(tools/DemoAnalyser)
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoDumper)
	add_subdirectory(tools/DemoDumper)
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoDumper)

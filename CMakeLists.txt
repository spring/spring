### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
project(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

### Install path (relative to CMAKE_INSTALL_PREFIX)
if    (UNIX AND NOT MINGW)
	set (BINDIR "bin" CACHE STRING "Where to install binaries")
	set (LIBDIR "lib" CACHE STRING "Where to install libraries")
	set (DATADIR "share/games/spring" CACHE STRING "Where to install game content")
	set (AIDIR "${LIBDIR}/spring" CACHE STRING "Where AIs are installed")

	set (APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set (PIXMAPS_DIR "share/pixmaps" CACHE STRING "Where to install icons")
	set (MIME_DIR "share/mime"CACHE STRING "Where MIME definitions are located")
	set (STABS_DEBUG_SYMBOLS FALSE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
else  (UNIX AND NOT MINGW)
	set (BINDIR "." CACHE STRING "Where to install binaries")
	set (LIBDIR "." CACHE STRING "Where to install libraries")
	set (DATADIR "." CACHE STRING "Where to install game content")
	set (AIDIR "." CACHE STRING "Where AIs are installed")

	set(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs package")
	if    (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
		MESSAGE(FATAL_ERROR "MINGWLIBS is not a valid directory")
	endif (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
	include_directories(${MINGWLIBS}/include)
	set(CMAKE_LIBRARY_PATH ${MINGWLIBS}/dll)
	set(CMAKE_PREFIX_PATH ${MINGWLIBS})
	
	# some cross-compile fixes
	set (CMAKE_FIND_ROOT_PATH ${MINGWLIBS} ${CMAKE_FIND_ROOT_PATH})
	#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	set (STABS_DEBUG_SYMBOLS TRUE CACHE BOOL "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version).")
endif (UNIX AND NOT MINGW)

### Usr-definable build options
set (AIINTERFACES "ALL" CACHE STRING "Which AI Interfaces (and AIs using them) to build [ALL|NATIVE|JAVA|NONE]")

set(SYNCCHECK TRUE CACHE BOOL "Check sync during gameplay (Required for online play!)")
if    (SYNCCHECK)
	add_definitions(-DSYNCCHECK)
endif (SYNCCHECK)

if    (WIN32)
	set(NO_AVI FALSE CACHE BOOL "Disable in-game video recording")
	if    (NO_AVI)
		add_definitions(-DNO_AVI)
	endif (NO_AVI)
	set(WINDOWS_CONSOLE FALSE CACHE BOOL "Show a visible cmd.exe with stdout when spring runs")
	if    (NOT WINDOWS_CONSOLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
	endif (NOT WINDOWS_CONSOLE)
else  (WIN32)
	add_definitions(-DNO_AVI)
endif (WIN32)

if (CMAKE_HOST_WIN32)
	set(PATH_DELIM ";")
else (CMAKE_HOST_WIN32)
	set(PATH_DELIM ":")
endif(CMAKE_HOST_WIN32)

if    (NOT MINGW)
	set (SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	set (SPRING_DATADIR_INTERNAL "")
	if    (SPRING_DATADIR)
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR}${PATH_DELIM}")
	endif (SPRING_DATADIR)
	if    (NOT "${DATADIR}" STREQUAL "${AIDIR}")
		set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}/${AIDIR}${PATH_DELIM}")
	endif (NOT "${DATADIR}" STREQUAL "${AIDIR}")
	set (SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}/${DATADIR}")
	add_definitions(-DSPRING_DATADIR="${SPRING_DATADIR_INTERNAL}")
endif (NOT MINGW)

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if    (CUSTOM_COMPILER_DEFINITIONS)
	add_definitions(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)


if    (MINGW)
	set(SDL_INCLUDE_DIR "${MINGWLIBS}/include/SDL")
endif (MINGW)
FIND_PACKAGE(SDL)
if    (SDL_FOUND)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
else  (SDL_FOUND)
	message("SDL not found on your system, disabling:")
	message("* spring (engine)")
	message("* spring-dedicated")
endif (SDL_FOUND)

FIND_PACKAGE(Boost 1.34.0 COMPONENTS thread regex program_options system REQUIRED)
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
INCLUDE(TestCXXAcceptsVisibilityFlag)
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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -mfpmath=sse") # SSE1
	if    (MINGW)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads") # SSE1
	endif (MINGW)

	set(CXX_FLAGS_DEBUG_ADDITIONAL "")
endif ($ENV{CXX} MATCHES "icpc")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math -fsignaling-nans")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee-fp -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
add_definitions(-DSTREFLOP_SSE) # would break AI compiling, but is undefined in ./AI/CMakeLists.txt
if    (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif (UNIX)
set(CMAKE_CXX_FLAGS_DEBUG          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O1 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG2         "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_DEBUG3         "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE        "-O2 -pg -Wall -DNDEBUG")
if    (STABS_DEBUG_SYMBOLS)
	set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}  -gstabs")
	set(CMAKE_CXX_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2} -gstabs")
	set(CMAKE_CXX_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3} -gstabs")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstabs")
else  (STABS_DEBUG_SYMBOLS)
	set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}  -ggdb1")
	set(CMAKE_CXX_FLAGS_DEBUG2         "${CMAKE_CXX_FLAGS_DEBUG2} -ggdb2")
	set(CMAKE_CXX_FLAGS_DEBUG3         "${CMAKE_CXX_FLAGS_DEBUG3} -ggdb3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
endif (STABS_DEBUG_SYMBOLS)

if     (NOT MINGW AND NOT APPLE)
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
if    (NOT "${AIINTERFACES}" STREQUAL "NONE")
	Add_Subdirectory(AI)
endif (NOT "${AIINTERFACES}" STREQUAL "NONE")
if    (SDL_FOUND)
	Add_Subdirectory(tools/DedicatedServer)
endif (SDL_FOUND)
Add_Subdirectory(tools/unitsync)

### make the basefiles (aka sdz's)
if    (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else  (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif (CMAKE_HOST_WIN32)

### install base data files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".gitignore" EXCLUDE)
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

if    (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)
	Add_Subdirectory(tools/DemoAnalyser)
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)

if    (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoDumper)
	Add_Subdirectory(tools/DemoDumper)
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoDumper)

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
else (UNIX AND NOT MINGW)
	set (BINDIR "." CACHE STRING "Where to install binaries")
	set (LIBDIR "." CACHE STRING "Where to install libraries")
	set (DATADIR "." CACHE STRING "Where to install game content")
	set (AIDIR "." CACHE STRING "Where AIs are installed")
	set (CMAKE_INSTALL_PREFIX "/")
endif (UNIX AND NOT MINGW)
set (AIINTERFACES "ALL" CACHE STRING "Which AI Interfaces (and AIs using them) to build [ALL|NATIVE|JAVA|NONE]")

### Usr-definable build options
SET(SYNCCHECK TRUE CACHE BOOL "Check sync during gameplay (Required for online play!)")
if (SYNCCHECK)
	ADD_DEFINITIONS(-DSYNCCHECK)
endif (SYNCCHECK)

SET(DIRECT_CONTROL TRUE CACHE BOOL "Allow controling units in first-person-mode")
if (DIRECT_CONTROL)
	ADD_DEFINITIONS(-DDIRECT_CONTROL_ALLOWED)
endif (DIRECT_CONTROL)

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

SET(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
if (SPRING_DATADIR)
	ADD_DEFINITIONS(-DSPRING_DATADIR="${SPRING_DATADIR}:${CMAKE_INSTALL_PREFIX}/${DATADIR}:${CMAKE_INSTALL_PREFIX}/${AIDIR}")
else (SPRING_DATADIR)
	ADD_DEFINITIONS(-DSPRING_DATADIR="${CMAKE_INSTALL_PREFIX}/${DATADIR}:${CMAKE_INSTALL_PREFIX}/${AIDIR}")
endif (SPRING_DATADIR)

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if (CUSTOM_COMPILER_DEFINITIONS)
	ADD_DEFINITIONS(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)


FIND_PACKAGE(SDL)
if (SDL_FOUND)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
else (SDL_FOUND)
	message("SDL not found on your system, disabling:")
	message("* spring (engine)")
	message("* AIs")
endif (SDL_FOUND)

FIND_PACKAGE(Boost 1.35.0 COMPONENTS thread regex REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(Devil REQUIRED)

### mingwlibs
if (MINGW)
	SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs (you can't build windows executables without them)")
	include_directories(${MINGWLIBS}/include)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)
	
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
#Add_Subdirectory(tools/DemoAnalyser)
#Add_Subdirectory(tools/DemoDumper)

### make the basefiles (aka sdz's)
if (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (CMAKE_HOST_WIN32)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh ${CMAKE_BINARY_DIR}/base WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(CMAKE_HOST_WIN32)

### install base data files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".gitignore" EXCLUDE)
install (DIRECTORY ${CMAKE_BINARY_DIR}/base DESTINATION ${DATADIR}/base)

### Install freedesktop-files, mime-types etc.
if (UNIX AND NOT MINGW)
	install (FILES installer/freedesktop/applications/spring.desktop DESTINATION ${APPLICATIONS_DIR})
	install (FILES installer/freedesktop/mime/spring.xml DESTINATION ${MIME_DIR}/packages)
	install (FILES installer/freedesktop/pixmaps/spring.png installer/freedesktop/pixmaps/application-x-spring-demo.png DESTINATION ${PIXMAPS_DIR})
else (UNIX AND NOT MINGW)
	install (DIRECTORY ${MINGWLIBS}/bin/ DESTINATION ${BINDIR} PATTERN "*.dll")
endif (UNIX AND NOT MINGW)

# additional tools

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)
	add_subdirectory(tools/DemoAnalyser)
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/DemoAnalyser)

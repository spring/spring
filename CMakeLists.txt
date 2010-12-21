
project(Spring)

# CMake 2.4:
# - lacks some cross-compiling features and fails on windows
# CMake 2.6:
# - Windows command line is limited to 32k length commands
# CMake 2.6.3:
# + support for SOURCES attribute of add_custom_target(), which is required for AI stuff
# CMake 2.8
# + works around command line length limited by using an @objectFilesList.txt parameter when linking
if    (WIN32)
	set(MIN_CMAKE_VERSION 2.8)
else  (WIN32)
	set(MIN_CMAKE_VERSION 2.6.3)
endif (WIN32)
cmake_minimum_required(VERSION ${MIN_CMAKE_VERSION})

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

# includes rts/build/cmake/Util.cmake
INCLUDE(Util)

CheckMinCMakeVersion(CUSTOM_TARGET_SOURCES_SUPPORT 2 6 3)
IF     (NOT CUSTOM_TARGET_SOURCES_SUPPORT)
	MESSAGE(
		"warning: ADD_CUSTOM_TARGET() support for the SOURCES attribute "
		"was added in CMake 2.6.3, you are using CMake "
		"${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}. "
		"Some parts of ${CMAKE_PROJECT_NAME} require this feature, "
		"and will consequently not be configured/built. "
		"You will get separate warnings for all parts that will not be built.")
ENDIF  ()

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

set(HEADLESS_SYSTEM FALSE CACHE BOOL "Compile for a headless system (default- and MT-spring will not be built!)")
SET(NO_SOUND FALSE CACHE BOOL "No sound output support")
if    (HEADLESS_SYSTEM AND NOT NO_SOUND)
	message(FATAL_ERROR "HEADLESS_SYSTEM requires NO_SOUND to be set!")
endif (HEADLESS_SYSTEM AND NOT NO_SOUND)

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
endif (MINGW)
if    (WIN32)
	FIND_PACKAGE(Windres)
endif (WIN32)

FIND_PACKAGE(Boost 1.35.0 COMPONENTS thread regex program_options system signals REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(DevIL REQUIRED)

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
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
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
# This should produce a sane amount of warnings
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

if     (MINGW)
	set (CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-auto-import")
endif  (MINGW)

IF (MINGW)
	FIND_PACKAGE(Win32Libs REQUIRED)
ENDIF (MINGW)

### Where the other CMakeLists.txt are

# As the libs are required by all the other sub-projects, we add them first
Add_Subdirectory(rts/lib)

if    (NOT "${AI_TYPES}" STREQUAL "NONE")
	Add_Subdirectory(AI)
endif (NOT "${AI_TYPES}" STREQUAL "NONE")

# additional tools
Add_Subdirectory(tools)

# man pages
Add_Subdirectory(doc)

### Handle game content
### - Pack base-files
### - Install base-files
### - Install freedesktop stuff
### - Install mingw DLLs
Add_Subdirectory(cont)

# As rts/builds/* depends on most other stuff, we add this one as last
Add_Subdirectory(rts)


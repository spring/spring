# CMake 2.4:
# - lacks some cross-compiling features and fails on windows
# CMake 2.6:
# - fixes: Windows command line is limited to 32k length commands
# CMake 2.6.3:
# + support for SOURCES attribute of add_custom_target() (not required anymore)
# CMake 2.8
# + works around command line length limited by using an @objectFilesList.txt parameter when linking
if    (WIN32)
	set(MIN_CMAKE_VERSION 2.8)
else  (WIN32)
	set(MIN_CMAKE_VERSION 2.6)
endif (WIN32)
cmake_minimum_required(VERSION ${MIN_CMAKE_VERSION})

SET(CMAKE_MODULES_SPRING "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_SPRING}")

# enable flag overrides for compilers
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_MODULES_SPRING}/OverrideFlags.cmake")

project(Spring)

# includes rts/build/cmake/Util.cmake & rts/build/cmake/PreferStaticLibs.cmake
INCLUDE(Util)
INCLUDE(PreferStaticLibs)

CheckMinCMakeVersion(CUSTOM_TARGET_SOURCES_SUPPORT 2 6 3)


# set some internal vars
If    ("${CMAKE_BUILD_TYPE}" MATCHES "^DEBUG")
	Set(DEBUG_BUILD TRUE)
Else  ("${CMAKE_BUILD_TYPE}" MATCHES "^DEBUG")
	Set(DEBUG_BUILD FALSE)
EndIf ("${CMAKE_BUILD_TYPE}" MATCHES "^DEBUG")

# MSVC needs the default flags
if(NOT MSVC)
	### Strip C[XX]_FLAGS
	option(CUSTOM_CFLAGS "If false all C[XX]_FLAGS are cleared (more reliable to sync/safer for online play)." FALSE)
	if    (CUSTOM_CFLAGS)
		Message(WARNING "Using custom CXX_FLAGS! this build will very likely not sync in online mode!")
	else  (CUSTOM_CFLAGS)
		set(CMAKE_C_FLAGS "")
		set(CMAKE_CXX_FLAGS "")
	endif (CUSTOM_CFLAGS)
endif(NOT MSVC)


set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if    (CUSTOM_COMPILER_DEFINITIONS)
	add_definitions(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)


### set the default build type
if    (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RELWITHDEBINFO CACHE STRING
		"Choose the type of build, options are: RELWITHDEBINFO DEBUG DEBUG2 DEBUG3 RELEASE PROFILE"
		FORCE)
endif (NOT CMAKE_BUILD_TYPE)


# Tell windows to pass arguments around in @response files
# (32k arg limit workaround)
if    (WIN32)
	SET(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
	SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
endif (WIN32)


# XCode
if    (APPLE)
	option(MACOSX_BUNDLE "Compile spring to work as a Bundle.app" TRUE)
	add_definitions(${PIC_FLAG})
	if    (MACOSX_BUNDLE)
		add_definitions(-DMACOSX_BUNDLE)
	endif (MACOSX_BUNDLE)
else  (APPLE)
	set(MACOSX_BUNDLE FALSE)
endif (APPLE)

if    (UNIX AND NOT MINGW)
	execute_process(COMMAND ${CMAKE_LINKER} "-v" COMMAND "grep" "-iq" "gold" RESULT_VARIABLE hasGold)
	if    (hasGold EQUAL 0)
		Message(STATUS "Found ld.gold, compressing debugsymbols")
		set(CMAKE_EXE_LINKER_FLAGS " -Wl,--compress-debug-sections=zlib ${CMAKE_EXE_LINKER_FLAGS}")
	else  (hasGold EQUAL 0)
		Message(WARNING "You are using ld.bfd, it's much slower than ld.gold.")
	endif (hasGold EQUAL 0)
endif (UNIX AND NOT MINGW)


### Install paths (relative to CMAKE_INSTALL_PREFIX)
if    (UNIX AND NOT MINGW)
	set(BINDIR  "bin"                CACHE STRING "Where to install binaries")
	set(LIBDIR  "lib"                CACHE STRING "Where to install libraries")
	set(MANDIR  "share/man"          CACHE STRING "Where to install man pages")
	set(DOCDIR  "share/doc/spring-VERSION" CACHE STRING "Where to install basic project documentation (README, LICENSE, etc.)") # TODO set correct version here, and re-enable userdocs in doc/CMakeLists.txt
	set(DATADIR "share/games/spring" CACHE STRING "Where to install game content")

	set(APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set(PIXMAPS_DIR      "share/pixmaps"      CACHE STRING "Where to install icons")
	set(MIME_DIR         "share/mime"         CACHE STRING "Where MIME definitions are located")
else  (UNIX AND NOT MINGW)
	set(BINDIR  "." CACHE STRING "Where to install binaries")
	set(LIBDIR  "." CACHE STRING "Where to install libraries")
	set(DOCDIR  "doc" CACHE STRING "Where to install basic project documentation (README, LICENSE, etc.)")
	set(DATADIR "." CACHE STRING "Where to install game content")

	if    (NOT MINGWLIBS)
		if(MSVC)
			set(MINGWLIBS ${CMAKE_SOURCE_DIR}/vclibs CACHE PATH "Location of the vclibs package")
		else(MSVC)
			set(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs package")
		endif(MSVC)
	endif (NOT MINGWLIBS)
	if    (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")
		MESSAGE(FATAL_ERROR "MINGWLIBS '${MINGWLIBS}' is not a valid directory")
	endif (NOT EXISTS "${MINGWLIBS}" OR NOT IS_DIRECTORY "${MINGWLIBS}")

	set(CMAKE_PREFIX_PATH ${MINGWLIBS})
	if   (MSVC)
		add_definitions(-D_WIN32_WINNT=0x502) ## vista is minimum due to InterlockedIncrement64
	else (MSVC)
		add_definitions(-D_WIN32_WINNT=0x500) ## win2k is minimum
	endif(MSVC)

	# some cross-compile fixes
	set(CMAKE_FIND_ROOT_PATH ${MINGWLIBS} ${CMAKE_FIND_ROOT_PATH})
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	if(MSVC)
		set(CMAKE_LIBRARY_PATH ${MINGWLIBS})
		set(CMAKE_FIND_ROOT_PATH
			${MINGWLIBS}/SDL
			${MINGWLIBS}/DevIL
			${MINGWLIBS}/zlib
			${MINGWLIBS}/ogg_vorbis
			${MINGWLIBS}/glew
			${MINGWLIBS}/freetype
			${CMAKE_FIND_ROOT_PATH})
		set(ENV{OPENALDIR} ${MINGWLIBS}/OpenAL)
		include_directories(${MINGWLIBS}/zlib/include)

		set(BOOST_LIBRARYDIR ${MINGWLIBS}/boost/lib)
		add_definitions("-DBOOST_ALL_NO_LIB")
		set(Boost_USE_STATIC_LIBS ON)
		set(GLEW_STATIC ON)

		set(EXE_FLAGS WIN32)
	else(MSVC)
		include_directories(${MINGWLIBS}/include)
		set(CMAKE_LIBRARY_PATH ${MINGWLIBS}/dll)
	endif(MSVC)
endif (UNIX AND NOT MINGW)

option(STABS_DEBUG_SYMBOLS "Use stabs debug symbols (-stabs) instead of the GDB format (-gdb). This probably makes gdb and eg. addr2line work better with windows binaries (depends mainly on the tools version)." FALSE)

## Artificial Intelligence
# In theory, it should be possible to set this to LIBDIR instead
# (next two are relative to CMAKE_INSTALL_PREFIX)
set(AI_LIBS_DIR "${DATADIR}"     CACHE STRING "Where to install Skirmish AI libraries")
set(AI_DATA_DIR "${AI_LIBS_DIR}" CACHE STRING "Where to install Skirmish AI additional files (eg. configuration)")
set(AI_TYPES    "ALL"            CACHE STRING "Which AI Interfaces (and Skirmish AIs using them) to build [ALL|NATIVE|JAVA|NONE]")

## DataDirs
set(BUILTIN_DATADIRS "")
if    (NOT WIN32 AND NOT MACOSX_BUNDLE)
	set(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	set(SPRING_DATADIR_INTERNAL "")
	if    (SPRING_DATADIR)
		set(SPRING_DATADIR_INTERNAL "${SPRING_DATADIR}${PATH_DELIM_T}")
	endif (SPRING_DATADIR)
	if    (CMAKE_INSTALL_PREFIX)
		set(SPRING_DATADIR_INTERNAL "${SPRING_DATADIR_INTERNAL}${CMAKE_INSTALL_PREFIX}${PATH_SEP_T}${DATADIR}")
	endif (CMAKE_INSTALL_PREFIX)
	set(BUILTIN_DATADIRS "${SPRING_DATADIR_INTERNAL}")
endif (NOT WIN32 AND NOT MACOSX_BUNDLE)
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



# Spring Related USE-Flags
set(LOG_LEVEL "DEFAULT" CACHE STRING
		"The minimum log level.
		Log messages below this level
		will not be compiled into the binary.
		Available levels:
		* DEFAULT (DEBUG on debug builds, INFO on release builds)
		* DEBUG (log everything)
		* INFO
		* WARNING (only use this if you know what you are doing!)")
if    (NOT "${LOG_LEVEL}" STREQUAL "DEFAULT")
	add_definitions(-D_LOG_LEVEL_MIN=LOG_LEVEL_${LOG_LEVEL})
endif (NOT "${LOG_LEVEL}" STREQUAL "DEFAULT")

option(SYNCCHECK "Check sync during gameplay (Required for online play!)" TRUE)
if    (SYNCCHECK)
	add_definitions(-DSYNCCHECK)
endif (SYNCCHECK)

option(NO_SOUND "No sound output support" FALSE)

if    (WIN32)
	## internal AVI recorder (w/o sound atm)
	option(NO_AVI "Disable in-game video recording" FALSE)
	if    (NOT NO_AVI)
		add_definitions(-DAVI_CAPTURING)
	endif (NOT NO_AVI)

	if   (NOT MSVC)
		## stdout console
		option(WINDOWS_CONSOLE "Show a visible cmd.exe with stdout when spring runs" FALSE)
		if    (NOT WINDOWS_CONSOLE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
		endif (NOT WINDOWS_CONSOLE)
	endif(NOT MSVC)
endif (WIN32)

option(HEADLESS_SYSTEM "Compile a headless executable (other executables will not be built!)" FALSE)
if    (HEADLESS_SYSTEM AND NOT NO_SOUND)
	message(FATAL_ERROR "HEADLESS_SYSTEM requires NO_SOUND to be set!")
endif (HEADLESS_SYSTEM AND NOT NO_SOUND)

option(USE_LIBSQUISH "Enable libsquish for DXT decompressing. (needed for ETC1 support)" TRUE)
if    (USE_LIBSQUISH)
	add_definitions(-DUSE_LIBSQUISH)
endif (USE_LIBSQUISH)


### external libs
if    (MINGW)
	set(SDL_INCLUDE_DIR "${MINGWLIBS}/include/SDL")
endif (MINGW)

FIND_PACKAGE(Boost 1.47.0 COMPONENTS thread regex program_options system chrono signals filesystem REQUIRED)
FIND_PACKAGE(Boost 1.47.0 COMPONENTS timer)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

# boost::chrono depends on clock_gettime which we need to link ourself when boost is static linked
Set(Boost_CHRONO_LIBRARY_WITH_RT ${Boost_CHRONO_LIBRARY})
IF    (PREFER_STATIC_LIBS AND UNIX AND NOT MINGW)
	FIND_LIBRARY(REALTIME_LIBRARY rt)
	LIST(APPEND Boost_CHRONO_LIBRARY_WITH_RT ${REALTIME_LIBRARY})
ENDIF (PREFER_STATIC_LIBS AND UNIX AND NOT MINGW)

# Note the missing REQUIRED, as headless & dedi may not depend on those.
#  So req. checks are done in the build target's CMakeLists.txt.
FIND_PACKAGE(SDL)
FIND_PACKAGE_STATIC(DevIL)
IF    (PREFER_STATIC_LIBS)
	# dependencies of DevIL
	FIND_PACKAGE_STATIC(JPEG REQUIRED)
	FIND_PACKAGE_STATIC(PNG REQUIRED)
	FIND_PACKAGE_STATIC(TIFF REQUIRED)
	FIND_PACKAGE_STATIC(GIF REQUIRED)
ENDIF (PREFER_STATIC_LIBS)

if    (WIN32)
	FIND_PACKAGE(Windres)
	SET(CMAKE_RC_COMPILER ${WINDRES_BIN})
	FIND_PACKAGE(Win32Libs REQUIRED)
endif (WIN32)

if     (WIN32)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_MODULE_PREFIX "")
elseif (UNIX)
	# unix specifics
else   (UNIX)
	# mac
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	INCLUDE_DIRECTORIES("${COREFOUNDATION_LIBRARY}/Headers")
endif  (WIN32)

FIND_PACKAGE(Valgrind)
if    (VALGRIND_FOUND)
	add_definitions(-DUSE_VALGRIND)
endif (VALGRIND_FOUND)


### Compiler flags and defines based on build type
INCLUDE(TestCXXFlags)

## 32bit or 64bit?
if    (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 64bit environment")
	set(MARCH_FLAG ${MARCH} CACHE STRING "CPU optimization (use i686 for generic optimization)")
	set(MARCH_BITS 64 INTERNAL)
else  (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message (STATUS "Building Spring for 32bit environment")
	set(MARCH_FLAG ${MARCH} CACHE STRING "CPU optimization (use i686 for generic optimization)")
	set(MARCH_BITS 32 INTERNAL)
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)


### Add Link-Time-Optimization flags, if supported (GCC >= 4.5) and enabled
Set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS}           ${LTO_FLAGS}")
Set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${LTO_FLAGS}")
Set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LTO_FLAGS}")


### C++11
option(USE_CXX11 "Enable c++11" TRUE)
if    (USE_CXX11)
	Message(STATUS "Compiling with C++11")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS}")
	add_definitions(-D_GLIBCXX_USE_NANOSLEEP) # issue in std::chrono with gentoo and few other distros
	add_definitions(-D_RANDOM_TCC) # don't include <bits/random.tcc>, it fails to compile with single precision constants
endif (USE_CXX11)


### our custom OpenMP replacement
add_definitions(-DTHREADPOOL)


if(NOT MSVC)
	### SigNan
	option(SIGNAL_NANS "Enable NaN-Signalling" ${DEBUG_BUILD})
	if (SIGNAL_NANS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsignaling-nans")
	endif(SIGNAL_NANS)


	### CompileTime Warnings
	set(COMMON_WARNINGS "")
	if    (NOT OPENMP)
		# disable warnings about unknown pragmas because of:
		# "#pragma omp parallel for *"
		set(COMMON_WARNINGS "${COMMON_WARNINGS} -Wno-unknown-pragmas")
	endif (NOT OPENMP)

	if    (MAX_WARNINGS)
		# This would produce the maximum on warnings
		set(DEBUG_WARNINGS "-Wall -Wconversion ${COMMON_WARNINGS}")
	else  (MAX_WARNINGS)
		# This should produce a sane amount of warnings
		set(DEBUG_WARNINGS "-Wall -Wno-sign-compare ${COMMON_WARNINGS}")
	endif (MAX_WARNINGS)
endif(NOT MSVC)

### CXX_FLAGS
set(CXX_FLAGS_DEBUG_ADDITIONAL "")
if    ($ENV{CXX} MATCHES "icpc")
	# intel C/C++ compiler fix; it does not support these flags:
	# "-march -mfpmath -msse -ggdb"
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -axK") # SSE1
	set(CXX_FLAGS_DEBUG_ADDITIONAL "-g -debug full")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	if    (NOT CUSTOM_CFLAGS)
		if    (MARCH_FLAG)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${MARCH_FLAG}")
			Message(STATUS "using march=${MARCH_FLAG}")
		else  (MARCH_FLAG)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
			Message(STATUS "using mtune=generic (unsure if it will sync in online gaming)")
		endif (MARCH_FLAG)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SSE_FLAGS}") # SSE1
	endif (NOT CUSTOM_CFLAGS)
elseif (MSVC)
	# nothing to be done here
else   (MSVC)
	Message(WARNING "unknown compiler")
endif ($ENV{CXX} MATCHES "icpc")

add_definitions(-DSTREFLOP_SSE) # would break AI compiling, but is undefined in ./AI/CMakeLists.txt

if    (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
else  (MSVC)
	if (CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math")
	endif(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IEEE_FP_FLAG} -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
	if (UNIX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	endif (UNIX)
	if    (MINGW)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
	endif (MINGW)

	add_definitions(-DBOOST_NO_FENV_H) # streflop overrides fenv.h

	set(CMAKE_CXX_FLAGS_DEBUG           "${CXX_FLAGS_DEBUG_ADDITIONAL} -O1     ${DEBUG_WARNINGS}  -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
	set(CMAKE_CXX_FLAGS_DEBUG2          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0     ${DEBUG_WARNINGS}  -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
	set(CMAKE_CXX_FLAGS_DEBUG3          "${CXX_FLAGS_DEBUG_ADDITIONAL} -O0     ${DEBUG_WARNINGS}  -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CXX_FLAGS_DEBUG_ADDITIONAL} -O2     ${COMMON_WARNINGS} -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE         "                              -O2     ${COMMON_WARNINGS} -DNDEBUG")
	set(CMAKE_CXX_FLAGS_PROFILE         "                              -O2 -pg ${DEBUG_WARNINGS}  -DNDEBUG -DPROFILE")
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
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
		if    (MARCH_BITS EQUAL 32)
			# Increase memory limit from 2GB to 3GB on 32bit Windows and 2GB->4GB on 64bit Windows (assuming spring.exe is 32bit)
			# http://msdn.microsoft.com/en-us/library/windows/desktop/aa366778(v=vs.85).aspx
			Message(STATUS "Enable IMAGE_FILE_LARGE_ADDRESS_AWARE (>2GB memory limit)")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--large-address-aware")
		endif (MARCH_BITS EQUAL 32)
	endif  (MINGW)
endif (MSVC)



#######################################
# Source Files
#

# Sub-parts of the build system may add dependencies to this target
add_custom_target(generateSources)

# Make sure git is found if git is used
If   (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	find_package(Git REQUIRED)
Endif(EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Generate the version files
Set(VERSION_GENERATE_DIR "${CMAKE_BINARY_DIR}")

Set_Source_Files_Properties(
		"${VERSION_GENERATE_DIR}/src-generated/engine/System/VersionGenerated.h"
		"${VERSION_GENERATE_DIR}/VERSION"
		PROPERTIES GENERATED TRUE
	)

# This is always executed (on every run of "make"),
# but the actual version files are only written in case of a change
# in the version info they use, or in the template file content.
Add_Custom_Target(generateVersionFiles)
Add_Custom_Command(TARGET generateVersionFiles
	COMMAND "${CMAKE_COMMAND}"
		"-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
		"-DCMAKE_MODULES_SPRING=${CMAKE_MODULES_SPRING}"
		"-DGENERATE_DIR=${VERSION_GENERATE_DIR}"
		"-DVERSION_ADDITIONAL=${VERSION_ADDITIONAL}"
		-P "${CMAKE_MODULES_SPRING}/ConfigureVersion.cmake"
		"${VERSION_GENERATE_DIR}"
	COMMENT
		"  Configuring Version files ..." VERBATIM
	)

Add_Dependencies(generateSources generateVersionFiles)


### Where the other CMakeLists.txt are

# As the libs are required by all the other sub-projects, we add them first
Add_Subdirectory(rts/lib)

if    (NOT "${AI_TYPES}" STREQUAL "NONE")
	Add_Subdirectory(AI)
endif (NOT "${AI_TYPES}" STREQUAL "NONE")

# As tools/unitsync/ depends on sources from here, we add this one first
Add_Subdirectory(rts/System)

# additional tools
Add_Subdirectory(tools)

# man pages & user-docs
Add_Subdirectory(doc)

### Handle game content
### - Pack base-files
### - Install base-files
### - Install freedesktop stuff
### - Install mingw DLLs
Add_Subdirectory(cont)

# As rts/builds/* depends on most other stuff, we add this one as last
Add_Subdirectory(rts)

# Unit tests
# this has to be in root CMakeLists.txt; it requires CMake 2.6.3 or later
ENABLE_TESTING()
Add_Subdirectory(test)

### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
project(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

### Install path (relative to CMAKE_INSTALL_PREFIX)
set (BINDIR "bin" CACHE STRING "Where to install binaries")
set (LIBDIR "lib" CACHE STRING "Where to install libraries")
set (DATADIR "share/games/spring" CACHE STRING "Where to install game content")
if (UNIX AND NOT MINGW)
	set (APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
	set (PIXMAPS_DIR "share/pixmaps" CACHE STRING "Where to install icons")
	set (MIME_DIR "share/mime"CACHE STRING "Where MIME definitions are located")
endif (UNIX AND NOT MINGW)

### Usr-definable build options
SET(SYNCCHECK TRUE CACHE BOOL "Check sync during gameplay (Required for online play!)")
if (SYNCCHECK)
	ADD_DEFINITIONS(-DSYNCCHECK)
endif (SYNCCHECK)

SET(DIRECT_CONTROL TRUE CACHE BOOL "Allow controling units in first-person-mode")
if (DIRECT_CONTROL)
	ADD_DEFINITIONS(-DDIRECT_CONTROL_ALLOWED)
endif (DIRECT_CONTROL)

IF (UNIX)
	ADD_DEFINITIONS(-DNO_AVI)
ELSEIF (UNIX)
	SET(NO_AVI FALSE CACHE BOOL "Disable in-game video recording")
	if (NO_AVI)
		ADD_DEFINITIONS(-DNO_AVI)
	endif (NO_AVI)
ENDIF(UNIX)

SET(SPRING_DATADIR ${CMAKE_INSTALL_PREFIX}/${DATADIR} CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
if (SPRING_DATADIR)
	ADD_DEFINITIONS(-DSPRING_DATADIR="${SPRING_DATADIR}")
endif (SPRING_DATADIR)

set (CUSTOM_COMPILER_DEFINITIONS "" CACHE STRING "Custom flags can be defined here")
if (CUSTOM_COMPILER_DEFINITIONS)
	ADD_DEFINITIONS(${CUSTOM_COMPILER_DEFINITIONS})
endif (CUSTOM_COMPILER_DEFINITIONS)

### mingwlibs
if (MINGW)
	SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs (you can't build windows executables without them)")
	include_directories(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)
else (MINGW)
	# AIs need it
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost 1.34.0 COMPONENTS thread regex REQUIRED)
endif (MINGW)

### Compiler flags and defines based on build type
INCLUDE(TestCXXAcceptsVisibilityFlag)
set (MARCH_FLAG native CACHE STRING "CPU optimization (use i686 for generic optimization)")
if (MARCH_FLAG)
	set (CMAKE_CXX_FLAGS "-march=${MARCH_FLAG} -mfpmath=387")
endif (MARCH_FLAG)

# intel C compiler fix (does not support these flags: -march -mfpmath -ggdb)
IF ($ENV{CXX} MATCHES "icpc")
	SET(CMAKE_CXX_FLAGS "")
	SET(CXX_FLAGS_DEBUG_ADDITIONAL "-g -debug full")
ENDIF ($ENV{CXX} MATCHES "icpc")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsingle-precision-constant -frounding-math -fsignaling-nans -mieee-fp -pipe -fno-strict-aliasing ${VISIBILITY_HIDDEN} ${VISIBILITY_INLINES_HIDDEN}")
# ADD_DEFINITIONS(-DSTREFLOP_X87) breaks AI compiling
IF (UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF (UNIX)
SET(CMAKE_CXX_FLAGS_DEBUG   "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb1 -O1 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG2  "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb2 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG3  "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb3 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_RELEASE "${BASE_FLAGS} ${VISIBILITY_HIDDEN} -O2 -DNDEBUG -fomit-frame-pointer")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_PROFILE   "-O2 -pg -Wall -DNDEBUG")
if (NOT MINGW)
	set (PIC_FLAG "-fpic")
endif (NOT MINGW)

### Where the other CMakeLists.txt are
Add_Subdirectory(rts)
Add_Subdirectory(AI)
Add_Subdirectory(tools/DedicatedServer)
Add_Subdirectory(tools/unitsync)
#Add_Subdirectory(tools/DemoAnalyser)
#Add_Subdirectory(tools/DemoDumper)

### make the basefiles (aka sdz's)
if (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(UNIX OR CMAKE_CROSSCOMPILING)

### install base data files
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".svn" EXCLUDE)

### Install freedesktop-files, mime-types etc.
if (UNIX AND NOT MINGW)
	install (FILES installer/freedesktop/applications/spring.desktop DESTINATION ${APPLICATIONS_DIR})
	install (FILES installer/freedesktop/mime/spring.xml DESTINATION ${MIME_DIR}/packages)
	install (FILES installer/freedesktop/pixmaps/spring.png installer/freedesktop/pixmaps/application-x-spring-demo.png DESTINATION ${PIXMAPS_DIR})
endif (UNIX AND NOT MINGW)


### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
Project(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

set (BINDIR "spring" CACHE STRING "Where to install binaries")
set (LIBDIR "spring" CACHE STRING "Where to install libraries")
set (DATADIR "spring" CACHE STRING "Where to install game content")

set (COMPLETE_BUILD true)

if (MINGW)
	SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs (you can't build windows executables without them)")
	include_directories(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)
endif (MINGW)

### Compiler flags and defines based on build type
INCLUDE(TestCXXAcceptsVisibilityFlag)
set (MARCH_FLAG native CACHE STRING "Passed to GCC for CPU-dependend optimization")
SET(CMAKE_CXX_FLAGS "-march=${MARCH_FLAG} -fsingle-precision-constant -frounding-math -fsignaling-nans -mieee-fp -mfpmath=387 -pipe -fno-strict-aliasing")
IF (UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF (UNIX)
SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS} -ggdb1 -O1 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG2  "${CMAKE_CXX_FLAGS} -ggdb2 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG3  "${CMAKE_CXX_FLAGS} -ggdb3 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${VISIBILITY_HIDDEN} -O2 -DNDEBUG -fomit-frame-pointer")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} ${VISIBILITY_HIDDEN} -g -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_PROFILE   "${CMAKE_CXX_FLAGS} -O2 -pg -Wall -DNDEBUG")

Add_Subdirectory(rts)
Add_Subdirectory(AI)
Add_Subdirectory(tools/DedicatedServer)
Add_Subdirectory(tools/unitsync)
Add_Subdirectory(tools/DemoAnalyser)

if (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(UNIX OR CMAKE_CROSSCOMPILING)

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".svn" EXCLUDE)



### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
Project(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rts/build/cmake")

set (ONE_DIRECTORY TRUE CACHE BOOLEAN "When true, place all spring files in one single directory, otherwise seperate to bin/, lib/ etc.")
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
SET(BASE_FLAGS "-fsingle-precision-constant -frounding-math -fsignaling-nans -mieee-fp -pipe -fno-strict-aliasing")
IF (NOT $ENV{CXX} MATCHES "icpc")
	SET(BASE_FLAGS "${BASE_FLAGS} -march=${MARCH_FLAG} -mfpmath=387")
ENDIF (NOT $ENV{CXX} MATCHES "icpc")
IF (UNIX)
	SET(BASE_FLAGS "${BASE_FLAGS} -pthread")
ENDIF (UNIX)
IF ($ENV{CXX} MATCHES "icpc")
	SET(CXX_FLAGS_DEBUG_ADDITIONAL "-g -debug full")
ENDIF ($ENV{CXX} MATCHES "icpc")
SET(CMAKE_CXX_FLAGS_DEBUG   "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb1 -O1 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG2  "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb2 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_DEBUG3  "${BASE_FLAGS} ${CXX_FLAGS_DEBUG_ADDITIONAL} -ggdb3 -O0 -Wall -DDEBUG -D_DEBUG -DNO_CATCH_EXCEPTIONS")
SET(CMAKE_CXX_FLAGS_RELEASE "${BASE_FLAGS} ${VISIBILITY_HIDDEN} -O2 -DNDEBUG -fomit-frame-pointer")
SET(CMAKE_CXX_FLAGS_PROFILE   "${BASE_FLAGS} -O2 -pg -Wall -DNDEBUG")

Add_Subdirectory(rts)
Add_Subdirectory(AI)
Add_Subdirectory(tools/DedicatedServer)
Add_Subdirectory(tools/unitsync)

if (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND sh make_gamedata_arch.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
else (UNIX OR CMAKE_CROSSCOMPILING)
	add_custom_target(gamedata ALL COMMAND call make_gamedata_arch.bat WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installer)
endif(UNIX OR CMAKE_CROSSCOMPILING)

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/game/ DESTINATION ${DATADIR} PATTERN ".svn" EXCLUDE)



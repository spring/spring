# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
STRING(REPLACE " -fsingle-precision-constant" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "Building Assimp with flags ${CMAKE_CXX_FLAGS}")

# Tell Assimp not to use Boost, it doesn't always match up with the version shipped with mingwlibs
SET (ENABLE_BOOST_WORKAROUND  "off" CACHE STRING "Disable Boost thread support in Assimp library")
IF (ENABLE_BOOST_WORKAROUND)
	INCLUDE_DIRECTORIES(code/BoostWorkaround)
	ADD_DEFINITIONS(-DASSIMP_BUILD_BOOST_WORKAROUND)
	MESSAGE(STATUS "Building a non-boost version of Assimp.")
ENDIF (ENABLE_BOOST_WORKAROUND)

# Assimp header files
INCLUDE_DIRECTORIES(include)

# Assimp code
ADD_SUBDIRECTORY(code)

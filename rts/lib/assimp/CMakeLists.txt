# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
SET(CMAKE_CXX_FLAGS "-g -c -DDEBUG -D_DEBUG -gstabs")

# Tell Assimp not to use Boost, it doesn't always match up with the version shipped with mingwlibs
#SET(ENABLE_BOOST_WORKAROUND "on")
#INCLUDE_DIRECTORIES(code/BoostWorkaround)
#ADD_DEFINITIONS(-DASSIMP_BUILD_BOOST_WORKAROUND)
#MESSAGE(STATUS "Building a non-boost version of Assimp.")

# Assimp header files
INCLUDE_DIRECTORIES(include)

# Assimp code
ADD_SUBDIRECTORY(code)

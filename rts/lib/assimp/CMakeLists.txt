cmake_minimum_required( VERSION 2.6 )
PROJECT( Assimp )

# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
SET(CMAKE_CXX_FLAGS "-g -c -DDEBUG -D_DEBUG -gstabs")

# Tell Assimp not to use Boost, it doesn't really match up with the version shipped with mingwlibs
SET(ENABLE_BOOST_WORKAROUND "on")

# Assimp header files
INCLUDE_DIRECTORIES( include )

# Assimp code
ADD_SUBDIRECTORY( code/ )
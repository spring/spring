cmake_minimum_required( VERSION 2.6 )
PROJECT( Assimp )

# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
SET(CMAKE_CXX_FLAGS "-g -c -DDEBUG -D_DEBUG -gstabs")

INCLUDE_DIRECTORIES( include )

# Libs
ADD_SUBDIRECTORY( code/ )
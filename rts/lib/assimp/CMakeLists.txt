CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( AssetImporter )

INCLUDE_DIRECTORIES( include ../streflop )

# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
SET(CMAKE_CXX_FLAGS "-g -c")

#SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY lib )
ADD_SUBDIRECTORY( code )


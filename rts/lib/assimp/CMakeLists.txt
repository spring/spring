cmake_minimum_required( VERSION 2.6 )
PROJECT( AssetImporter )

# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
SET(CMAKE_CXX_FLAGS "-g -c -DDEBUG -D_DEBUG -gstabs")

#SET( CMAKE_CURRENT_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/code )
#SET( CMAKE_CURRENT_BINARY_DIR ${CMAKE_HOME_DIRECTORY}/bin )

INCLUDE_DIRECTORIES( 
	include
)

#SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
#SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin ) 

# Libs
add_subdirectory( code/ )

#IF( WIN32 )
#	add_subdirectory( tools/assimp_view/ )
#ENDIF( WIN32 )




# Spring's compiler flags break assimp (especially -fsingle-precision-constants)
STRING(REPLACE " -fsingle-precision-constant" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "Building Assimp with flags ${CMAKE_CXX_FLAGS}")

# Define here the needed parameters
set (ASSIMP_VERSION_MAJOR 2)
set (ASSIMP_VERSION_MINOR 0)
set (ASSIMP_VERSION_PATCH 0)
set (ASSIMP_VERSION ${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}.${ASSIMP_VERSION_PATCH})
set (ASSIMP_SOVERSION ${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR})
SET ( PROJECT_VERSION "${ASSIMP_SOVERSION}" )

set(PACKAGE_VERSION "0" CACHE STRING "the package-specific version used for uploading the sources")
set(BUILD_STATIC_LIB 1)
# Assimp code
ADD_SUBDIRECTORY(code)

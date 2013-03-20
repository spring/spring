# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


### Assemble defines
ADD_DEFINITIONS(-DUSE_GML)

if   (NOT MSVC)
	# USE_GML requires this
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-tls-direct-seg-refs")
endif(NOT MSVC)

# USE_GML_SIM requires USE_GML
ADD_DEFINITIONS(-DUSE_GML_SIM)

if (USE_GML_DEBUG)
	ADD_DEFINITIONS(-DUSE_GML_DEBUG)
endif (USE_GML_DEBUG)


### Add icon and manifest to exe using windres
IF    (WIN32)
	IF   (MINGW)
		SET(ENGINE_ICON_GML_DIR "${ENGINE_SRC_ROOT_DIR}")
		SET(ENGINE_ICON_GML_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
		SET(ENGINE_ICON_GML_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
		CreateResourceCompileCommand(ENGINE_ICON_GML "${ENGINE_ICON_GML_DIR}" "${ENGINE_ICON_GML_RES}" "${ENGINE_ICON_GML_OBJ}")
	ENDIF(MINGW)
ELSE  (WIN32)
	SET(ENGINE_ICON_GML "")
ENDIF (WIN32)

### Assemble external incude dirs (defined in ../default/CMakeLists.txt)
INCLUDE_DIRECTORIES(${engineIncludes})

### Build the executable
ADD_EXECUTABLE(engine-multithreaded ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON_GML} ${engineHeaders})
TARGET_LINK_LIBRARIES(engine-multithreaded gml ${engineLibraries})
SET_TARGET_PROPERTIES(engine-multithreaded PROPERTIES OUTPUT_NAME "spring")


### Install the executable
INSTALL(TARGETS engine-multithreaded DESTINATION ${BINDIR})

# Only build & install spring-multithreaded executable & dependencies
# use cases:
# * make spring-multithreaded
# * make install-spring-multithreaded
CreateEngineBuildAndInstallTarget(multithreaded)


# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


ADD_DEFINITIONS(-DUSE_GML)
# USE_GML requires this
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-tls-direct-seg-refs")

# USE_GML_SIM requires USE_GML
ADD_DEFINITIONS(-DUSE_GML_SIM)

if (USE_GML_DEBUG)
	ADD_DEFINITIONS(-DUSE_GML_DEBUG)
endif (USE_GML_DEBUG)


### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_GML_DIR "${ENGINE_SRC_ROOT_DIR}")
	SET(ENGINE_ICON_GML_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	SET(ENGINE_ICON_GML_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_GML_ICON "${ENGINE_ICON_GML_DIR}" "${ENGINE_ICON_GML_RES}" "${ENGINE_ICON_GML_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_GML_ICON "")
ENDIF (MINGW)


ADD_EXECUTABLE(spring-gml EXCLUDE_FROM_ALL ${engineSources} ${ENGINE_ICON_GML} ${engineHeaders})
TARGET_LINK_LIBRARIES(spring-gml gml ${engineLibraries})

# Note: installing a target that has EXCLUDE_FROM_ALL set is illegal
#INSTALL(TARGETS spring-gml DESTINATION ${BINDIR})


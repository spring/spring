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
# USE_GML requires this
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-tls-direct-seg-refs")

# USE_GML_SIM requires USE_GML
ADD_DEFINITIONS(-DUSE_GML_SIM)

if (USE_GML_DEBUG)
	ADD_DEFINITIONS(-DUSE_GML_DEBUG)
endif (USE_GML_DEBUG)


### Assemble libraries
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
LIST(APPEND engineLibraries ${SDL_LIBRARY})

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
LIST(APPEND engineLibraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES})

FIND_PACKAGE(Freetype REQUIRED)
LIST(APPEND engineLibraries ${FREETYPE_LIBRARIES})

IF    (UNIX)
	FIND_PACKAGE(X11 REQUIRED)
	LIST(APPEND engineLibraries ${X11_X11_LIB} ${X11_Xcursor_LIB})
ENDIF (UNIX)

LIST(APPEND engineLibraries ${sound-impl})

LIST(APPEND engineLibraries ${engineCommonLibraries})


### Assemble external incude dirs
INCLUDE_DIRECTORIES(${DEVIL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})


### Add icon and manifest to exe using windres
IF    (WIN32)
	SET(ENGINE_ICON_GML_DIR "${ENGINE_SRC_ROOT_DIR}")
	SET(ENGINE_ICON_GML_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	SET(ENGINE_ICON_GML_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_ICON_GML "${ENGINE_ICON_GML_DIR}" "${ENGINE_ICON_GML_RES}" "${ENGINE_ICON_GML_OBJ}")
ELSE  (WIN32)
	SET(ENGINE_ICON_GML "")
ENDIF (WIN32)


### Build the executable
ADD_EXECUTABLE(spring-multithreaded ${engineSources} ${ENGINE_ICON_GML} ${engineHeaders})
TARGET_LINK_LIBRARIES(spring-multithreaded gml ${engineLibraries})


### Install the executable
INSTALL(TARGETS spring-multithreaded DESTINATION ${BINDIR})

# Only install spring-multithreaded executable & dependencies
# use case: make install-spring-multithreaded
CreateEngineBuildInstallTarget(spring-multithreaded)


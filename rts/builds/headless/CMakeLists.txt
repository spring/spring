# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

ADD_DEFINITIONS(-DHEADLESS)
ADD_DEFINITIONS(-DNO_SOUND)
ADD_DEFINITIONS(-DBITMAP_NO_OPENGL)
REMOVE_DEFINITIONS(-DAVI_CAPTURING)

include_directories(${OPENAL_INCLUDE_DIR})
IF    (MINGW OR APPLE OR MSVC)
	# Windows:
	# We still need these header files,
	# even if we are not going to link with gl, glu and SDL.
	# We have them available anyway (mingwlibs).
	# OS X:
	# Cocoa requires the SDL libary, whenever the SDL headers are used,
	# due to some #define magic, which is practically impossible to workaround.
	FIND_PACKAGE(OpenGL REQUIRED)
	FIND_PACKAGE(SDL2 REQUIRED)
	INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR})
ELSE  ()
	# Use a direct copy of the GL and SDL headers,
	# as these may not be available on headless systems.
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/SDL2)
ENDIF ()


# headlessstubs are our stubs that replace libGL, libGLU, libGLEW, libSDL (yes really!)
LIST(APPEND engineHeadlessLibraries headlessStubs)
LIST(APPEND engineHeadlessLibraries ${SPRING_SIM_LIBRARIES})
LIST(APPEND engineHeadlessLibraries engineSystemNet)
LIST(APPEND engineHeadlessLibraries ${engineCommonLibraries})
LIST(APPEND engineHeadlessLibraries engineaGui)
LIST(APPEND engineHeadlessLibraries no-sound)
LIST(APPEND engineHeadlessLibraries engineSim)
LIST(APPEND engineHeadlessLibraries pr-downloader_static)

INCLUDE_DIRECTORIES(${ENGINE_SRC_ROOT_DIR}/lib/assimp/include)


### Build the executable
ADD_EXECUTABLE(engine-headless ${engineSources} ${ENGINE_ICON})
TARGET_LINK_LIBRARIES(engine-headless no-sound ${engineHeadlessLibraries} no-sound)

IF    (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(engine-headless PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
ENDIF (MINGW)


### Install the executable
INSTALL(TARGETS engine-headless DESTINATION ${BINDIR})

# Only build & install spring-headless executable & dependencies
# use cases:
# * make spring-headless
# * make install-spring-headless
CreateEngineBuildAndInstallTarget(headless)


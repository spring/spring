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
REMOVE_DEFINITIONS(-DAVI_CAPTURING)

# We still need these header files,
# even if we are not going to link with gl, glu and SDL
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

# headlessstubs are our stubs that replace libGL, libGLU, libGLEW, libSDL (yes really!)
LIST(APPEND engineHeadlessLibraries headlessStubs)

LIST(APPEND engineHeadlessLibraries no-sound)

LIST(APPEND engineHeadlessLibraries ${engineCommonLibraries})

### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_HL_DIR "${ENGINE_SRC_ROOT_DIR}")
	SET(ENGINE_ICON_HL_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	SET(ENGINE_ICON_HL_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_ICON_HL "${ENGINE_ICON_HL_DIR}" "${ENGINE_ICON_HL_RES}" "${ENGINE_ICON_HL_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_ICON_HL "")
ENDIF (MINGW)


### Build the executable
ADD_EXECUTABLE(spring-hl ${engineSources} ${ENGINE_ICON_HL})
TARGET_LINK_LIBRARIES(spring-hl ${engineHeadlessLibraries})

IF    (MINGW)
	# To enable console output/force a console window to open
	SET_TARGET_PROPERTIES(spring-hl PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")

	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)


### Install the executable
INSTALL(TARGETS spring-hl DESTINATION ${BINDIR})

# Only install spring-hl executable & dependencies
# use case: make install-spring-hl
CreateEngineBuildInstallTarget(spring-hl)


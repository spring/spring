# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

ADD_DEFINITIONS(-DHEADLESS)
REMOVE_DEFINITIONS(-DAVI_CAPTURING)

# headlessstubs are our stubs that replace libGL, libGLU, libSDL (yes really!)
# and also certain functions in libGLEW (we still link with libGLEW anyway)
LIST(APPEND engineHeadlessLibraries headlessStubs)

LIST(APPEND engineHeadlessLibraries ${engineCommonLibraries})

### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_HL_DIR "${ENGINE_SRC_ROOT_DIR}")
	SET(ENGINE_ICON_HL_RES "${ENGINE_SRC_ROOT_DIR}/icon.rc")
	SET(ENGINE_ICON_HL_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_HL_ICON "${ENGINE_ICON_HL_DIR}" "${ENGINE_ICON_HL_RES}" "${ENGINE_ICON_HL_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_ICON_HL "")
ENDIF (MINGW)

ADD_EXECUTABLE(spring-hl EXCLUDE_FROM_ALL ${engineSources} ${ENGINE_ICON_HL} ${engineHeaders})
TARGET_LINK_LIBRARIES(spring-hl ${engineHeadlessLibraries})

IF    (MINGW)
	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	SET_TARGET_PROPERTIES(spring-hl PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
	#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)

# Note: installing a target that has EXCLUDE_FROM_ALL set is illegal
#INSTALL(TARGETS spring-hl DESTINATION ${BINDIR})


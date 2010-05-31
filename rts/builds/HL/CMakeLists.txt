INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ System)

# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

SET(SPRINGRTSDIR "${CMAKE_SOURCE_DIR}/rts")


FIND_PACKAGE(OpenAL REQUIRED)
LIST(APPEND spring_headlessstubs_libraries ${OPENAL_LIBRARY})
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
FIND_PACKAGE(OggVorbis REQUIRED)
LIST(APPEND spring_headlessstubs_libraries ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})

FIND_PACKAGE(Freetype REQUIRED)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
LIST(APPEND spring_headlessstubs_libraries ${FREETYPE_LIBRARIES})

### Find include directories and add platform specific libraries
IF    (MINGW)
	FIND_PACKAGE(Win32Libs REQUIRED)
	LIST(APPEND spring_headlessstubs_libraries ${WIN32_LIBRARIES} mingw32)
ELSE  (MINGW)
	FIND_PACKAGE(X11 REQUIRED)
	LIST(APPEND spring_headlessstubs_libraries ${X11_X11_LIB} ${X11_Xcursor_LIB})
ENDIF (MINGW)

LIST(APPEND spring_headlessstubs_libraries ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_SIGNALS_LIBRARY})

# headlessstubs are our stubs that replace libGL, libGLU, libSDL (yes really!)
# and also certain functions in libGLEW (we still link with libGLEW anyway)
LIST(APPEND spring_headlessstubs_libraries headlessStubs lua 7zip oscpack minizip streflop lobby)
#IF (USE_GML)
#	list (APPEND spring_libraries gml)
#ENDIF (USE_GML)

# we still need the header files, even if we are not going to link
# with gl, glu, or glut
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
# we still link with glew
# (you try stubbing the whole of libGLEW ;-) Good luck with that :-P)
LIST(APPEND spring_headlessstubs_libraries ${GLEW_LIBRARIES})

LIST(APPEND spring_headlessstubs_libraries ${DEVIL_IL_LIBRARY} ${DEVIL_ILU_LIBRARY})

# include directories needed for compiling spring
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR}/lib/lua/include)

# need to add this so stuff compiles:
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR})
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR}/System)

### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_HL_DIR "${SPRINGRTSDIR}")
	SET(ENGINE_ICON_HL_RES "${SPRINGRTSDIR}/icon.rc")
	SET(ENGINE_ICON_HL_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_HL_ICON "${ENGINE_ICON_HL_DIR}" "${ENGINE_ICON_HL_RES}" "${ENGINE_ICON_HL_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_HL_ICON "")
ENDIF (MINGW)

ADD_DEFINITIONS(-DHEADLESS)
ADD_EXECUTABLE(spring-hl EXCLUDE_FROM_ALL ${allspringfiles} ${ENGINE_HL_ICON})
TARGET_LINK_LIBRARIES(spring-hl ${spring_headlessstubs_libraries})

IF    (MINGW)
	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	SET_TARGET_PROPERTIES(spring-hl PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
	#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)

# Note: installing a target that has EXCLUDE_FROM_ALL set is illegal
#INSTALL(TARGETS spring-hl DESTINATION ${BINDIR})


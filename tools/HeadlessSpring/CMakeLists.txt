INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ System)

# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
# modified output directory to point to same one as spring's:
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "..")

set(THISSCRIPTDIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

set(SPRINGRTSDIR "${CMAKE_SOURCE_DIR}/rts")

### Spring defines
SET(USE_MMGR FALSE CACHE BOOL "Use memory manager?")
if (USE_MMGR)
	ADD_DEFINITIONS(-DUSE_MMGR)
endif (USE_MMGR)

SET(USE_GML FALSE CACHE BOOL "Use the GML OpenGL threading library?")
if (USE_GML)
	ADD_DEFINITIONS(-DUSE_GML)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-tls-direct-seg-refs")
endif (USE_GML)

SET(USE_GML_SIM FALSE CACHE BOOL "Use independent Simulation thread?")
if (USE_GML_SIM)
	ADD_DEFINITIONS(-DUSE_GML_SIM)
endif (USE_GML_SIM)

SET(USE_GML_DEBUG FALSE CACHE BOOL "Use GML call debugging?")
if (USE_GML_DEBUG)
	ADD_DEFINITIONS(-DUSE_GML_DEBUG)
endif (USE_GML_DEBUG)

SET(TRACE_SYNC FALSE CACHE BOOL "Enable sync tracker")
if (TRACE_SYNC)
	ADD_DEFINITIONS(-DTRACE_SYNC)
endif (TRACE_SYNC)

SET(SYNCDEBUG FALSE CACHE BOOL "Enable sync debugger (needs SYNCCHECK=true)")
if (SYNCDEBUG)
	ADD_DEFINITIONS(-DSYNCDEBUG)
endif (SYNCDEBUG)

FIND_PACKAGE(OpenAL REQUIRED)
LIST(APPEND spring_headlessstubs_libraries ${OPENAL_LIBRARY})
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
FIND_PACKAGE(OggVorbis REQUIRED)
LIST(APPEND spring_headlessstubs_libraries ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})

FIND_PACKAGE(Freetype REQUIRED)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
LIST(APPEND spring_headlessstubs_libraries ${FREETYPE_LIBRARIES})

### Find include directories and add platform specific libraries
IF (MINGW)
	FIND_PACKAGE(Win32Libs REQUIRED)
	LIST(APPEND spring_headlessstubs_libraries ${WIN32_LIBRARIES} mingw32)
ELSE (MINGW)
	FIND_PACKAGE(X11 REQUIRED)
	LIST(APPEND spring_headlessstubs_libraries ${X11_X11_LIB} ${X11_Xcursor_LIB})
ENDIF (MINGW)

LIST(APPEND spring_headlessstubs_libraries ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_SIGNALS_LIBRARY})

# headlessstubs is our stubs that replace libGL, libGLU, libSDL( yes really!) and
# also replaces certain functions in libGLEW (we still link with libGLEW anyway)
LIST(APPEND spring_headlessstubs_libraries headlessstubs lua 7zip oscpack minizip streflop lobby)
#if (USE_GML)
#	list (APPEND spring_libraries gml)
#endif (USE_GML)

ADD_SUBDIRECTORY(lib) # contains the headlessstubs library

# we still need the header files, even if we're not going to link with gl, glu, or glut
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
# we still link with glew (you try stubbing the whole of libGLEW ;-) Good luck with that :-P)
LIST(APPEND spring_headlessstubs_libraries ${GLEW_LIBRARIES})

LIST(APPEND spring_headlessstubs_libraries ${DEVIL_IL_LIBRARY} ${DEVIL_ILU_LIBRARY})

# include directories needed for compiling spring
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR}/lib/lua/include)

# need to add this so stuff compiles:
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR})
INCLUDE_DIRECTORIES(BEFORE ${SPRINGRTSDIR}/System)

### Generate icon to exe
IF (MINGW)
	SET(ICON_SRC_DIR  ${CMAKE_SOURCE_DIR}/rts)
	SET(ICON_OBJ_FILE ${CMAKE_CURRENT_BINARY_DIR}/icon.o)
	ADD_CUSTOM_COMMAND(OUTPUT ${ICON_OBJ_FILE} COMMAND ${WINDRES} -I${ICON_SRC_DIR} -i${ICON_SRC_DIR}/icon.rc -o ${ICON_OBJ_FILE} -v)
	SET_SOURCE_FILES_PROPERTIES(${ICON_OBJ_FILE} PROPERTIES
		GENERATED      TRUE
		OBJECT_DEPENDS ${ICON_SRC_DIR}/icon.rc)
ENDIF (MINGW)

ADD_DEFINITIONS(-DHEADLESS)
ADD_EXECUTABLE(spring-hl ${allspringfiles})
ADD_DEPENDENCIES(spring-hl headlessstubs )
TARGET_LINK_LIBRARIES(spring-hl ${spring_headlessstubs_libraries} )

IF (MINGW)
	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	SET_TARGET_PROPERTIES(spring-hl PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
	#install (FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)

install (TARGETS spring-hl DESTINATION ${BINDIR})


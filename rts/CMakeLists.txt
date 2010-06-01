INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/ System)

# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

### Spring defines
SET(USE_MMGR FALSE CACHE BOOL "Use memory manager?")
if (USE_MMGR)
	ADD_DEFINITIONS(-DUSE_MMGR)
endif (USE_MMGR)

SET(TRACE_SYNC FALSE CACHE BOOL "Enable sync tracker")
if (TRACE_SYNC)
	ADD_DEFINITIONS(-DTRACE_SYNC)
endif (TRACE_SYNC)

SET(SYNCDEBUG FALSE CACHE BOOL "Enable sync debugger (needs SYNCCHECK=true)")
if (SYNCDEBUG)
	ADD_DEFINITIONS(-DSYNCDEBUG)
endif (SYNCDEBUG)

# Only used by GML build, but used in builds/GML and lib/gml
SET(USE_GML_DEBUG FALSE CACHE BOOL "Use GML call debugging?")


FIND_PACKAGE(OpenAL REQUIRED)
LIST(APPEND spring_libraries ${OPENAL_LIBRARY})
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
FIND_PACKAGE(OggVorbis REQUIRED)
LIST(APPEND spring_libraries ${OGG_LIBRARY} ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY})

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLU REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
LIST(APPEND spring_libraries ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARIES} ${DEVIL_IL_LIBRARY})

FIND_PACKAGE(Freetype REQUIRED)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
LIST(APPEND spring_libraries ${FREETYPE_LIBRARIES})

### Find include directories and add platform specific libraries
IF (MINGW)
	FIND_PACKAGE(Win32Libs REQUIRED)
	LIST(APPEND spring_libraries ${WIN32_LIBRARIES} mingw32)
ELSE (MINGW)
	FIND_PACKAGE(X11 REQUIRED)
	LIST(APPEND spring_libraries ${X11_X11_LIB} ${X11_Xcursor_LIB})
ENDIF (MINGW)

LIST(APPEND spring_libraries ${SDL_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_SIGNALS_LIBRARY})

### build all libraries in lib (has its own CMakeLists.txt)
ADD_SUBDIRECTORY(lib)
LIST(APPEND spring_libraries lua 7zip oscpack minizip streflop lobby)

### include directories needed for compiling spring
INCLUDE_DIRECTORIES(BEFORE lib/lua/include)

### all spring source directories
### each directorie which contains source has to be here
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Game gamefiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Game/Camera gamefiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Game/Server gamefiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Game/StartScripts gamefiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Game/UI gamefiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Menu gamefiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Lua luafiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/ExternalAI aifiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/ExternalAI/Interface aifiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Map mapfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Map/SM3 mapfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Map/SM3/terrain mapfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Map/SMF mapfiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering renderfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Env renderfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering/GL renderfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Shaders renderfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Textures renderfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Models renderfiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/aGui renderfiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Features simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Misc simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/MoveTypes simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/MoveTypes/MoveMath simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Objects simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Path simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Projectiles simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Projectiles/WeaponProjectiles simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Projectiles/Unsynced simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Units simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Units/COB simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Units/CommandAI simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Units/Groups simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Units/UnitTypes simfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Sim/Weapons simfiles)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Platform sysfiles)
### only use the directory for target platform
IF (UNIX)
	AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Platform/Linux sysfiles)
ELSE (UNIX)
	AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Platform/Win sysfiles)
ENDIF (UNIX)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/FileSystem sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Net sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Script sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Sync sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Sound sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/creg sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/Input sysfiles)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/System/LoadSave sysfiles)

### Add icon and manifest to exe using windres
IF    (MINGW)
	SET(ENGINE_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	SET(ENGINE_ICON_RES "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
	SET(ENGINE_ICON_OBJ "${CMAKE_CURRENT_BINARY_DIR}/icon.o")
	CreateResourceCompileCommand(ENGINE_ICON "${ENGINE_ICON_DIR}" "${ENGINE_ICON_RES}" "${ENGINE_ICON_OBJ}")
ELSE  (MINGW)
	SET(ENGINE_ICON "")
ENDIF (MINGW)

### Add headers for generated project files (e.g. Code::Blocks)
IF (NOT WIN32)
	EXECUTE_PROCESS(COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} \( -name *.h -o -name *.hpp \) -printf %p\; OUTPUT_VARIABLE headers)
ENDIF (NOT WIN32)

SET( allspringfiles ${gamefiles} ${luafiles} ${mapfiles} ${fsfiles} ${renderfiles} ${simfiles} ${sysfiles} ${aifiles} ${nedmalloc_obj})
SET( allspringfiles ${allspringfiles} PARENT_SCOPE )

ADD_EXECUTABLE(spring ${allspringfiles} ${headers} ${ENGINE_ICON})
TARGET_LINK_LIBRARIES(spring ${spring_libraries})

IF (MINGW)
	#SET_TARGET_PROPERTIES(spring PROPERTIES LINK_FLAGS "-Wl,--output-def,spring.def")
	#install (FILES ${CMAKE_CURRENT_BINARY_DIR}/spring.def DESTINATION ${BINDIR})
ENDIF (MINGW)

install (TARGETS spring DESTINATION ${BINDIR})

Add_Subdirectory(builds)

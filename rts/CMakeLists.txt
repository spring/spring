### Spring defines
SET(USE_MMGR FALSE CACHE BOOL "Use memory manager?")
if (USE_MMGR)
	ADD_DEFINITIONS(-DUSE_MMGR)
endif (USE_MMGR)

SET(USE_GML FALSE CACHE BOOL "Use the GML OpenGL threading library?")
if (USE_GML)
	ADD_DEFINITIONS(-DUSE_GML)
endif (USE_GML)

IF (UNIX)
	ADD_DEFINITIONS(-DNO_AVI)
ELSEIF (UNIX)
	SET(NO_AVI FALSE CACHE BOOL "Disable in-game video recording")
	if (NO_AVI)
		ADD_DEFINITIONS(-DNO_AVI)
	endif (NO_AVI)
ENDIF(UNIX)

ADD_DEFINITIONS(-DSTREFLOP_X87)

### Find include directories and add platform specific libraries
IF (MINGW)
	if (NOT COMPLETE_BUILD)
		SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs (you can't build windows executables without them)")
		
		### add mingwlibs to link- and include-directories
		INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
		LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)
	endif (NOT COMPLETE_BUILD)
	FIND_PACKAGE(Win32Libs REQUIRED)
	LIST(APPEND spring_libraries glu32 ilu opengl32 ${WIN32_LIBRARIES} glew32 mingw32 devil)
	LIST(APPEND spring_libraries boost_thread-mt boost_regex-mt)
ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost 1.34.0 COMPONENTS thread regex REQUIRED)

	FIND_PACKAGE(X11 REQUIRED)
	FIND_PACKAGE(OpenAL REQUIRED)
	FIND_PACKAGE(OpenGL REQUIRED)
	FIND_PACKAGE(GLU REQUIRED)
	FIND_PACKAGE(GLUT REQUIRED)
	FIND_PACKAGE(GLEW REQUIRED)
	FIND_PACKAGE(Devil REQUIRED)
	FIND_PACKAGE(OggVorbis REQUIRED)

	FIND_PACKAGE(Freetype REQUIRED)
	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
	LIST(APPEND spring_libraries openal GL GLU ${DEVIL_LIBRARIES} X11 Xcursor ${GLEW_LIBRARIES} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY})
ENDIF (MINGW)

### libraries needed on all platforms
LIST(APPEND spring_libraries SDL vorbisfile vorbis ogg freetype)

### build all libraries in lib (has its own CMakeLists.txt)
ADD_SUBDIRECTORY(lib)
LIST(APPEND spring_libraries lua 7zip hpiutil2 minizip streflop luabind)
if (USE_GML)
	list (APPEND spring_libraries gml)
endif (USE_GML)

### include directories needed for compiling spring
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} System)
INCLUDE_DIRECTORIES(lib/lua/include lib/luabind)

### all spring source directories
### each directorie which contains source has to be here
AUX_SOURCE_DIRECTORY(Game gamefiles)
AUX_SOURCE_DIRECTORY(Game/Camera gamefiles)
AUX_SOURCE_DIRECTORY(Game/Server gamefiles)
AUX_SOURCE_DIRECTORY(Game/StartScripts gamefiles)
AUX_SOURCE_DIRECTORY(Game/UI gamefiles)

AUX_SOURCE_DIRECTORY(Lua luafiles)

AUX_SOURCE_DIRECTORY(ExternalAI aifiles)

AUX_SOURCE_DIRECTORY(Map mapfiles)
AUX_SOURCE_DIRECTORY(Map/SM3 mapfiles)
AUX_SOURCE_DIRECTORY(Map/SM3/terrain mapfiles)
AUX_SOURCE_DIRECTORY(Map/SMF mapfiles)

AUX_SOURCE_DIRECTORY(Rendering renderfiles)
AUX_SOURCE_DIRECTORY(Rendering/Env renderfiles)
AUX_SOURCE_DIRECTORY(Rendering/GL renderfiles)
AUX_SOURCE_DIRECTORY(Rendering/Textures renderfiles)
AUX_SOURCE_DIRECTORY(Rendering/UnitModels renderfiles)

AUX_SOURCE_DIRECTORY(Sim simfiles)
AUX_SOURCE_DIRECTORY(Sim/Features simfiles)
AUX_SOURCE_DIRECTORY(Sim/Misc simfiles)
AUX_SOURCE_DIRECTORY(Sim/MoveTypes simfiles)
AUX_SOURCE_DIRECTORY(Sim/MoveTypes/MoveMath simfiles)
AUX_SOURCE_DIRECTORY(Sim/Objects simfiles)
AUX_SOURCE_DIRECTORY(Sim/Path simfiles)
AUX_SOURCE_DIRECTORY(Sim/Projectiles simfiles)
AUX_SOURCE_DIRECTORY(Sim/Projectiles/WeaponProjectiles simfiles)
AUX_SOURCE_DIRECTORY(Sim/Projectiles/Unsynced simfiles)
AUX_SOURCE_DIRECTORY(Sim/Units simfiles)
AUX_SOURCE_DIRECTORY(Sim/Units/COB simfiles)
AUX_SOURCE_DIRECTORY(Sim/Units/CommandAI simfiles)
AUX_SOURCE_DIRECTORY(Sim/Units/UnitTypes simfiles)
AUX_SOURCE_DIRECTORY(Sim/Weapons simfiles)

AUX_SOURCE_DIRECTORY(System sysfiles)
AUX_SOURCE_DIRECTORY(System/Platform sysfiles)
### only use the directory for target platform
IF (UNIX)
	AUX_SOURCE_DIRECTORY(System/Platform/Linux sysfiles)
ELSE (UNIX)
	AUX_SOURCE_DIRECTORY(System/Platform/Win sysfiles)
ENDIF (UNIX)
AUX_SOURCE_DIRECTORY(System/FileSystem sysfiles)
AUX_SOURCE_DIRECTORY(System/Net sysfiles)
AUX_SOURCE_DIRECTORY(System/Script sysfiles)
AUX_SOURCE_DIRECTORY(System/Sync sysfiles)
AUX_SOURCE_DIRECTORY(System/creg sysfiles)

### Add icon and manifest to exe using windres
IF (MINGW)
	FIND_PROGRAM(WINDRES NAMES windres i686-mingw32-windres DOC "path to mingw's windres executable")
	ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon.o COMMAND ${WINDRES} -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/icon.rc -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o)
	LIST(APPEND sysfiles icon.o)
ENDIF (MINGW)

ADD_EXECUTABLE(spring ${gamefiles} ${luafiles} ${mapfiles} ${fsfiles} ${renderfiles} ${simfiles} ${sysfiles} ${aifiles})

TARGET_LINK_LIBRARIES(spring ${spring_libraries})
install (TARGETS spring DESTINATION ${BINDIR})

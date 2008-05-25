### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
PROJECT(Spring)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build/cmake")

### Compiler flags and defines based on build type
INCLUDE(TestCXXAcceptsVisibilityFlag)
SET(BASE_FLAGS "-march=i686 -fsingle-precision-constant -frounding-math -fsignaling-nans -mieee-fp -mfpmath=387 -pipe -fno-strict-aliasing")
IF (UNIX)
	SET(BASE_FLAGS "${BASE_FLAGS} -pthread")
ENDIF (UNIX)
SET(CMAKE_CXX_FLAGS_DEBUG1  "${BASE_FLAGS} -ggdb1 -O1 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG2  "${BASE_FLAGS} -ggdb2 -O0 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG3  "${BASE_FLAGS} -ggdb3 -O0 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "${BASE_FLAGS} ${VISIBILITY_HIDDEN} -O2 -DNDEBUG")

### Spring defines
ADD_DEFINITIONS( -D_SZ_ONE_DIRECTORY -DSYNCCHECK -DSTREFLOP_X87 -DDIRECT_CONTROL_ALLOWED)
IF (UNIX)
	ADD_DEFINITIONS(-DNO_AVI)
ELSE(UNIX)
ENDIF(UNIX)

### Find include directories
IF (MINGW)
	SET(MINGWLIBS ${CMAKE_SOURCE_DIR}/mingwlibs CACHE PATH "Location of the mingwlibs (you can't build windows executables without them)")
	FIND_PACKAGE(Win32Libs REQUIRED)

	### add mingwlibs to link- and include-directories
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)
ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost 1.34.0 COMPONENTS thread regex REQUIRED)
	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})

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
ENDIF (MINGW)

# build all libraries in lib (has its own CMakeLists.txt)
ADD_SUBDIRECTORY(lib)

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

### libraries in lib/
SET(spring_libraries lua 7zip hpiutil2 minizip streflop luabind)
### libraries needed on all platforms
# ${BOOST_THREAD_LIBRARY} doesn't work
LIST(APPEND spring_libraries SDL boost_thread-mt boost_regex-mt vorbisfile vorbis ogg freetype)

### platform specific libraries
IF (UNIX)
	LIST(APPEND spring_libraries openal GL GLU IL ILU X11 Xcursor ${GLEW_LIBRARIES})
ELSE (UNIX)
	LIST(APPEND spring_libraries glu32 ilu opengl32 ${WIN32_LIBRARIES} glew32 mingw32 devil)
ENDIF (UNIX)

TARGET_LINK_LIBRARIES(spring ${spring_libraries})

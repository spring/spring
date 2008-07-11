### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
PROJECT(Dedicated)

IF (MINGW)
	if (NOT COMPLETE_BUILD)
		SET(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	endif (NOT COMPLETE_BUILD)
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)

ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost 1.34.0 COMPONENTS thread regex REQUIRED)
ENDIF (MINGW)

SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=i686 -DNDEBUG")

ADD_DEFINITIONS( -DSTREFLOP_X87 -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK -fPIC)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/System/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

if (NOT COMPLETE_BUILD)
	ADD_SUBDIRECTORY(../../rts/lib libs)
endif (NOT COMPLETE_BUILD)

IF (UNIX)
	SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DataDirLocater ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles ../../rts/System/Platform/Win/WinFileSystemHandler ../../rts/System/Platform/Win/DataDirLocater ../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(system_files 
	${fsfiles}
	${platformfiles}
	 ${netfiles}
	../../rts/System/TdfParser
	../../rts/System/Platform/FileSystem
	../../rts/System/Platform/ConfigHandler
	../../rts/System/LogOutput
	../../rts/System/BaseNetProtocol
	../../rts/System/Demo
	../../rts/System/DemoReader
	../../rts/System/AutohostInterface
	../../rts/System/UnsyncedRNG )

AUX_SOURCE_DIRECTORY(../../rts/Game/Server serverfiles)
ADD_LIBRARY(server SHARED
	${system_files}
	${serverfiles}
	../../rts/Game/GameServer
	../../rts/Game/GameSetup
	../../rts/Game/GameData
	../../rts/Game/GameVersion
	../../rts/Game/CommandMessage
	../../rts/Game/ChatMessage
	../../rts/Game/Console
	../../rts/Game/Action
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/TAPalette)
TARGET_LINK_LIBRARIES(server SDL boost_thread-mt hpiutil2 7zip minizip lua boost_regex-mt)
IF (MINGW)
	TARGET_LINK_LIBRARIES(server ws2_32)
ENDIF (MINGW)

ADD_EXECUTABLE(dedicated main)
TARGET_LINK_LIBRARIES(dedicated server)

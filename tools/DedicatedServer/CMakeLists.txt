### Cmake 2.4 lacks some cross-compiling features and fails on windows
cmake_minimum_required(VERSION 2.6)
PROJECT(Dedicated)

IF (MINGW)
	SET(SPRING_DATADIR CACHE STRING "Path to game content (in addition to /etc/spring/datadir)")
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)

ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost REQUIRED)
	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})
ENDIF (MINGW)

SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=i686 -DNDEBUG")

ADD_DEFINITIONS( -DSTREFLOP_X87 -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})
TARGET_LINK_LIBRARIES(net SDL boost_thread-mt)
IF (MINGW)
	TARGET_LINK_LIBRARIES(net ws2_32)
ENDIF (MINGW)
INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/lib/lua/include)

ADD_SUBDIRECTORY(../../rts/lib libs)

IF (UNIX)
	SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DataDirLocater ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles ../../rts/System/Platform/Win/WinFileSystemHandler ../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(system_files ${fsfiles} ${platformfiles} ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/LogOutput)

AUX_SOURCE_DIRECTORY(../../rts/Game/Server serverfiles)
ADD_LIBRARY(server SHARED ${system_files} ${serverfiles} ../../rts/Game/GameServer GameSetup ../../rts/Game/GameSetupData ../../rts/Game/GameData ../../rts/Game/GameVersion ../../rts/Game/CommandMessage ../../rts/Game/ChatMessage ../../rts/Game/Console ../../rts/Game/Action ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/System/AutohostInterface  ../../rts/System/BaseNetProtocol ../../rts/System/UnsyncedRNG ../../rts/Lua/LuaParser ../../rts/Lua/LuaUtils)
TARGET_LINK_LIBRARIES(server SDL boost_thread-mt net hpiutil2 7zip minizip lua boost_regex-mt)

ADD_EXECUTABLE(dedicated main)
TARGET_LINK_LIBRARIES(dedicated server)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

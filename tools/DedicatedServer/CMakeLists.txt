IF (UNIX)
	OPTION(MINGW "If MinGW is used for building" FALSE)
ELSE (UNIX)
	OPTION(MINGW "If MinGW is used for building" TRUE)
ENDIF (UNIX)


IF (MINGW)
	SET(MINGWLIBS ../../mingwlibs CACHE PATH "Location of the mingwlibs")
	INCLUDE_DIRECTORIES(${MINGWLIBS}/include ${MINGWLIBS}/include/SDL)
	LINK_DIRECTORIES(${MINGWLIBS}/lib ${MINGWLIBS}/dll)

ELSE (MINGW)
	FIND_PACKAGE(SDL REQUIRED)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

	FIND_PACKAGE(Boost REQUIRED)
	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})
ENDIF (MINGW)

SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=i686 -msse -DNDEBUG")

ADD_DEFINITIONS( -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})
TARGET_LINK_LIBRARIES(net SDL boost_thread-mt)
IF (MINGW)
	TARGET_LINK_LIBRARIES(net ws2_32)
ENDIF (MINGW)
INCLUDE_DIRECTORIES(../../rts/System/Net/)

AUX_SOURCE_DIRECTORY(../../rts/lib/7zip 7zipfiles)
ADD_LIBRARY(7zip SHARED  ${7zipfiles})

IF (NOT MINGW)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/ioapi)
	TARGET_LINK_LIBRARIES(minizip z)
ELSE (NOT MINGW)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/iowin32 ../../rts/lib/minizip/ioapi)
	TARGET_LINK_LIBRARIES(minizip z)
ENDIF (NOT MINGW)

AUX_SOURCE_DIRECTORY(../../rts/lib/hpiutil2 hpiutil2)
ADD_LIBRARY(hpiutil2 SHARED  ${hpiutil2})
TARGET_LINK_LIBRARIES(hpiutil2 z)

IF (UNIX)
	SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DataDirLocater ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles ../../rts/System/Platform/Win/WinFileSystemHandler ../../rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(filesystem_files ${fsfiles} ${platformfiles} ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/LogOutput)

AUX_SOURCE_DIRECTORY(../../rts/Game/Server serverfiles)
ADD_LIBRARY(server SHARED ${filesystem_files} ${serverfiles} ../../rts/Game/GameServer GameSetup ../../rts/Game/GameSetupData ../../rts/Game/GameData ../../rts/Game/GameVersion ../../rts/Game/CommandMessage ../../rts/Game/ChatMessage ../../rts/Game/Console ../../rts/Game/Action ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/System/AutohostInterface  ../../rts/System/BaseNetProtocol ../../rts/System/UnsyncedRNG)
TARGET_LINK_LIBRARIES(server SDL boost_thread-mt net hpiutil2 7zip minizip boost_regex-mt)

ADD_EXECUTABLE(dedicated main)
TARGET_LINK_LIBRARIES(dedicated server)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

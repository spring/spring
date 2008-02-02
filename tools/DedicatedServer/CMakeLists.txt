FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

FIND_PACKAGE(Boost REQUIRED)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -DDEBUG -D_DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=i686 -msse -DNDEBUG")

ADD_DEFINITIONS( -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})
TARGET_LINK_LIBRARIES(net SDL boost_thread)
INCLUDE_DIRECTORIES(../../rts/System/Net/)

AUX_SOURCE_DIRECTORY(../../rts/lib/7zip 7zipfiles)
ADD_LIBRARY(7zip SHARED  ${7zipfiles})

IF (UNIX)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/ioapi)
	TARGET_LINK_LIBRARIES(minizip z)
ELSE (UNIX)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/iowin32)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/lib/hpiutil2 hpiutil2)
ADD_LIBRARY(hpiutil2 SHARED  ${hpiutil2})

IF (UNIX)
	SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DataDirLocater ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
	SET(platformfiles ../../rts/System/Platform/Windows/WinFileSystemHandler ../../rts/System/Platform/Windows/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(filesystem_files ${fsfiles} ${platformfiles} ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/LogOutput)

ADD_LIBRARY(server SHARED ${filesystem_files} ../../rts/Game/GameServer GameSetup ../../rts/Game/GameSetupData ../../rts/Game/GameVersion ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/System/AutohostInterface  ../../rts/System/BaseNetProtocol ../../rts/System/UnsyncedRNG)
TARGET_LINK_LIBRARIES(server SDL boost_thread net hpiutil2 7zip minizip boost_regex)

ADD_EXECUTABLE(dedicated main)
TARGET_LINK_LIBRARIES(dedicated server)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

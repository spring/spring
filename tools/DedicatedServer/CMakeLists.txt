PROJECT(DedicatedServer)
SET(CMAKE_CXX_FLAGS "-g -O1 -Wall")
ADD_DEFINITIONS(-DDEBUG -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})
TARGET_LINK_LIBRARIES(net SDL boost_thread)
INCLUDE_DIRECTORIES(../../rts/System/Net/ /usr/include/SDL/)

AUX_SOURCE_DIRECTORY(../../rts/lib/7zip 7zipfiles)
ADD_LIBRARY(7zip SHARED  ${7zipfiles})

IF (UNIX)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/ioapi)
	TARGET_LINK_LIBRARIES(minizip z)
ELSE (UNIX)
	ADD_LIBRARY(minizip SHARED  ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/iowin32)
	#TARGET_LINK_LIBRARIES(minizip z)
ENDIF (UNIX)



AUX_SOURCE_DIRECTORY(../../rts/lib/hpiutil2 hpiutil2)
ADD_LIBRARY(hpiutil2 SHARED  ${hpiutil2})

IF (UNIX)
SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DotfileHandler)
ELSE (UNIX)
SET(platformfiles ../../rts/System/Platform/Windows/WinFileSystemHandler ../../rts/System/Platform/Windows/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
ADD_LIBRARY(fileops SHARED  ${fsfiles} ${platformfiles} ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/LogOutput)
TARGET_LINK_LIBRARIES(fileops hpiutil2 7zip minizip boost_regex)

ADD_EXECUTABLE(server main GameServer ../../rts/Game/GameSetupData ../../rts/Game/GameVersion ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/System/AutohostInterface  ../../rts/System/BaseNetProtocol)
TARGET_LINK_LIBRARIES(server SDL boost_thread net fileops)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

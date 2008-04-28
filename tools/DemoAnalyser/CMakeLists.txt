PROJECT(DemoAnalyser)
SET(CMAKE_CXX_FLAGS "-O1 -Wall -g")

ADD_DEFINITIONS(-DDEBUG -D_DEBUG -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

INCLUDE_DIRECTORIES(/usr/include/SDL/)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System)

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
SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DotfileHandler ../../rts/System/Platform/Linux/DataDirLocater)
ELSE (UNIX)
SET(platformfiles ../../rts/System/Platform/Windows/WinFileSystemHandler ../../rts/System/Platform/Windows/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
ADD_LIBRARY(fileops SHARED  ${fsfiles} ${platformfiles} ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/LogOutput ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/Game/GameVersion)
TARGET_LINK_LIBRARIES(fileops hpiutil2 7zip minizip boost_regex boost_thread)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})

ADD_EXECUTABLE(analyser main)
TARGET_LINK_LIBRARIES(analyser fileops net SDL)
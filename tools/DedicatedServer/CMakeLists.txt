PROJECT(DedicatedServer)
SET(CMAKE_CXX_FLAGS "-g -O1 -Wall")
ADD_DEFINITIONS(-DDEBUG -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED )

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})
TARGET_LINK_LIBRARIES(net SDL boost_thread)
INCLUDE_DIRECTORIES(../../rts/System/Net/ /usr/include/SDL/)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
AUX_SOURCE_DIRECTORY(../../rts/lib/7zip 7zipfiles)
AUX_SOURCE_DIRECTORY(../../rts/lib/hpiutil2 hpiutil2)
ADD_LIBRARY(fileops SHARED  ${fsfiles} ${7zipfiles} ${hpiutil2} ../../rts/lib/minizip/unzip ../../rts/lib/minizip/zip ../../rts/lib/minizip/ioapi ../../rts/System/TdfParser ../../rts/System/Platform/FileSystem ../../rts/System/Platform/ConfigHandler ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DotfileHandler ../../rts/System/LogOutput)
TARGET_LINK_LIBRARIES(fileops boost_regex z)

ADD_EXECUTABLE(server main GameServer ../../rts/Game/GameVersion ../../rts/System/DemoReader ../../rts/System/Demo ../../rts/System/AutohostInterface  ../../rts/System/BaseNetProtocol)
TARGET_LINK_LIBRARIES(server SDL boost_thread net fileops)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/System ../unitsync)

PROJECT(DemoAnalyser)
SET(CMAKE_CXX_FLAGS "-O1 -Wall -g")

ADD_DEFINITIONS(-DDEBUG -D_DEBUG -D_SZ_ONE_DIRECTORY -DNO_AVI -DDEDICATED -DSYNCCHECK )

INCLUDE_DIRECTORIES(/usr/include/SDL/)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/lib/lua/include ../../rts/System)

IF (UNIX)
SET(platformfiles ../../rts/System/Platform/Linux/UnixFileSystemHandler ../../rts/System/Platform/Linux/DotfileHandler ../../rts/System/Platform/Linux/DataDirLocater)
ELSE (UNIX)
SET(platformfiles ../../rts/System/Platform/Windows/WinFileSystemHandler ../../rts/System/Platform/Windows/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
ADD_LIBRARY(fileops SHARED  ${fsfiles} ${platformfiles}
../../rts/Game/GameSetup
../../rts/Game/PlayerBase
../../rts/System/TdfParser
../../rts/System/Platform/FileSystem
../../rts/System/Platform/ConfigHandler
../../rts/System/LogOutput
../../rts/System/DemoReader
../../rts/System/Demo
../../rts/System/UnsyncedRNG
../../rts/Game/GameVersion
../../rts/Lua/LuaParser
../../rts/Lua/LuaUtils
../../rts/Map/MapParser
../../rts/Rendering/Textures/TAPalette
../../rts/Lua/LuaIO)
TARGET_LINK_LIBRARIES(fileops hpiutil2 7zip lua minizip boost_regex boost_thread)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
ADD_LIBRARY(net SHARED  ${netfiles})

ADD_EXECUTABLE(analyser EXCLUDE_FROM_ALL main)
TARGET_LINK_LIBRARIES(analyser fileops net SDL)
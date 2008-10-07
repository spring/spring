ADD_DEFINITIONS(-DDEDICATED)

INCLUDE_DIRECTORIES(/usr/include/SDL/)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/lib/lua/include ../../rts/System)

IF (UNIX)
	SET(platformfiles ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/UnixFileSystemHandler ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/DotfileHandler ${CMAKE_SOURCE_DIR}/rts/System/Platform/Linux/DataDirLocater)
ELSE (UNIX)
	SET(platformfiles ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/WinFileSystemHandler ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/DataDirLocater ${CMAKE_SOURCE_DIR}/rts/System/Platform/Win/RegHandler)
ENDIF (UNIX)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(build_files ${fsfiles} ${platformfiles}
../../rts/Game/GameSetup
../../rts/Game/PlayerBase
../../rts/System/TdfParser
../../rts/System/Platform/FileSystem
../../rts/System/Platform/ConfigHandler
../../rts/System/Net/RawPacket
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

ADD_EXECUTABLE(analyser EXCLUDE_FROM_ALL main ${build_files})
TARGET_LINK_LIBRARIES(analyser SDL hpiutil2 7zip lua minizip boost_regex boost_thread)
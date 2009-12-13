ADD_DEFINITIONS(-DDEDICATED -DNOT_USING_CREG)

INCLUDE_DIRECTORIES(../../rts/ ../../rts/lib/7zip ../../rts/lib/lua/include ../../rts/System)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(build_files ${fsfiles}
	../../rts/Game/GameSetup
	../../rts/Game/PlayerBase
	../../rts/System/Platform/Misc
	../../rts/System/TdfParser
	../../rts/System/ConfigHandler
	../../rts/System/Net/RawPacket
	../../rts/System/LogOutput
	../../rts/System/DemoReader
	../../rts/System/Demo
	../../rts/System/UnsyncedRNG
	../../rts/Game/GameVersion
	../../rts/Sim/Misc/TeamBase
	../../rts/Sim/Misc/AllyTeam
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/TAPalette
	../../rts/Lua/LuaIO)
IF (WIN32)
	SET(build_files ${build_files}
		../../rts/System/Platform/Win/WinVersion)
ENDIF (WIN32)

ADD_EXECUTABLE(demodumper EXCLUDE_FROM_ALL DemoDumper ${build_files})
TARGET_LINK_LIBRARIES(demodumper ${SDL_LIBRARY} hpiutil2 7zip lua minizip ${Boost_FILESYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})


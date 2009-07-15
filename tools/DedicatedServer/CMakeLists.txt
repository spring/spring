ADD_DEFINITIONS(-DDEDICATED ${PIC_FLAG} -DNO_AVI -DNOT_USING_CREG)
remove_definitions(-DSTREFLOP_SSE)

AUX_SOURCE_DIRECTORY(../../rts/System/Net/ netfiles)
INCLUDE_DIRECTORIES(../../rts/System/Net/ ../../rts/System/ ../../rts/lib/lua/include)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/lib/7zip ../../rts/System)

AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(system_files
	${fsfiles}
	${netfiles}
	../../rts/System/Platform/Misc
	../../rts/System/TdfParser
	../../rts/System/ConfigHandler
	../../rts/System/LogOutput
	../../rts/System/TimeUtil
	../../rts/System/BaseNetProtocol
	../../rts/System/Demo
	../../rts/System/DemoReader
	../../rts/System/DemoRecorder
	../../rts/System/AutohostInterface
	../../rts/System/UnsyncedRNG)

AUX_SOURCE_DIRECTORY(../../rts/Game/Server serverfiles)
ADD_LIBRARY(springserver SHARED
	${system_files}
	${serverfiles}
	../../rts/Game/GameServer
	../../rts/Game/ClientSetup
	../../rts/Game/GameSetup
	../../rts/Game/GameData
	../../rts/Game/PlayerBase
	../../rts/Game/GameVersion
	../../rts/Game/CommandMessage
	../../rts/Game/ChatMessage
	../../rts/Game/Console
	../../rts/Game/Action
	../../rts/Sim/Misc/TeamBase
	../../rts/Sim/Misc/AllyTeam
	../../rts/Lua/LuaIO
	../../rts/Lua/LuaParser
	../../rts/Lua/LuaUtils
	../../rts/Map/MapParser
	../../rts/Rendering/Textures/TAPalette)
TARGET_LINK_LIBRARIES(springserver ${SDL_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY}  ${Boost_SYSTEM_LIBRARY} hpiutil2 7zip minizip lua)
if (MINGW)
	TARGET_LINK_LIBRARIES (springserver ws2_32)
else (MINGW)
	set_target_properties(springserver PROPERTIES COMPILE_FLAGS "-fvisibility=default")
endif (MINGW)

ADD_EXECUTABLE(spring-dedicated main)
TARGET_LINK_LIBRARIES(spring-dedicated springserver)

install (TARGETS springserver spring-dedicated RUNTIME DESTINATION ${BINDIR} LIBRARY DESTINATION ${LIBDIR})
